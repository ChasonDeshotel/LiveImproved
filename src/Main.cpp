#ifndef TEST_BUILD
#include <JuceHeader.h>
#endif
//#include <dispatch/dispatch.h>
//#include <unistd.h>
#include <chrono>
#include <future>
#include <thread>
#include <utility>

#include "LogGlobal.h"
#include "PathFinder.h"

#include "DependencyContainer.h"
#include "PlatformInitializer.h"

#include "IEventHandler.h"
#include "IIPCCore.h"
#include "ILiveInterface.h"
#include "IPCCore.h"
#include "IPCResilienceDecorator.h"

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "ConfigMenu.h"
#include "EventHandler.h"
#include "KeySender.h"
#include "LimLookAndFeel.h"
#include "LiveInterface.h"
#include "PID.h"
#include "PluginManager.h"
#include "ResponseParser.h"
#include "Theme.h"
#include "WindowManager.h"


class JuceApp : public juce::JUCEApplication {
private:
    DependencyContainer& container_;
    std::unique_ptr<LimLookAndFeel> limLookAndFeel_;

    static constexpr int RESTART_DELAY_MS = 5000;
    static constexpr int LIVE_LAUNCH_DELAY = 10;
    static constexpr int DEFAULT_IPC_DELAY = 5;

    std::optional<std::filesystem::path> configFilePath_;
    std::optional<std::filesystem::path> configMenuPath_;

public:
    JuceApp()
        : container_(DependencyContainer::getInstance())
    {
        ::initializeLogger();
    }

    auto getApplicationName() -> const juce::String override {
        return "Live Improved";
    }

    auto getApplicationVersion() -> const juce::String override {
        return "0.69.420";
    }

    void initialise(const juce::String& commandLineArgs = "") override {
        //
        // TODO add check for is LES running
        //
        std::locale::global(std::locale("en_US.UTF-8"));

        logger->info("Ignition sequence started...");

        juce::LookAndFeel::setDefaultLookAndFeel(limLookAndFeel_.get());

        DependencyRegisterer r(std::make_shared<JuceApp>());
        r.eventHandler();

        // If Live already exists, go straight to launch
        // If not, add a the app launch callback
        // TODO needs a isRunning for multiple instances?
        if (PID::getInstance().livePID() == -1) {
            container_.resolve<IEventHandler>()->registerAppLaunch([this]() {
                logger->info("launch callback called");
                // delay to let Live fully start up
                juce::Thread::sleep(LIVE_LAUNCH_DELAY);
                this->onLiveLaunch(2);
            });
        } else {
            onLiveLaunch(DEFAULT_IPC_DELAY);
        }

        container_.resolve<IEventHandler>()->registerAppTermination([this]() {
            restartApplication();
        });
    }

    void onLiveLaunch(int ipcCallDelay) {
        logger->info("onLiveLaunch() called");

        DependencyRegisterer r(std::make_shared<JuceApp>());
        r.configFiles();
        r.theme();
        r.liveInterfaceAndStartObservers();
        r.responseParser();
        r.keySender();
        r.ipc();
        r.pluginManager();
        r.actionHandler();
        r.windowManager();

        if (ipcCallDelay > 0) juce::Thread::sleep(ipcCallDelay);

        logger->info("writing READY");
        container_.resolve<IIPCCore>()->writeRequest("READY", [this](const std::string& response) {
            logger->info("received READY response: " + response);
        });

        // TODO: IPC queue should be able to handle more writes without sleep
        juce::Thread::sleep(2);
        logger->info("refreshing plugin cache");
        container_.resolve<IPluginManager>()->refreshPlugins();

        #ifndef _WIN32
        PlatformInitializer::init();
        container_.resolve<IEventHandler>()->setupQuartzEventTap();
        PlatformInitializer::run();
        #endif
    }

    void restartApplication() {
        juce::String executablePath = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName();
        logger->info("executable path: " + executablePath.toStdString());

        // Spawn a new process to re-launch the application
        juce::ChildProcess process;
        if (process.start(executablePath)) {
            logger->info("Restarting application...");

            // Terminate the current process after the new one starts
            juce::Thread::sleep(RESTART_DELAY_MS);
            juce::JUCEApplicationBase::quit();
            // std::exit(0) for the hammer

        } else {
            logger->error("Failed to start new process");
        }
    }

    void shutdown() override {
        logger->info("shutdown() called");
        try {
            auto ipc = this->container_.resolve<IIPCCore>();
            if (ipc) {
                logger->info("stopping IPC...");
                ipc->stopIPC();

                std::promise<void> closePromise;
                std::future<void> closeFuture = closePromise.get_future();

                std::thread([&closePromise, ipc, this]() {
                    ipc->closeAndDeletePipes();
                    closePromise.set_value();  // Notify that pipes are closed
                }).detach();
                closeFuture.wait();
            }
        } catch (const std::exception& e) {
            logger->error("Failed to resolve IIPCCore: " + std::string(e.what()));
        } catch (...) {
            logger->error("Unknown error occurred while resolving IIPCCore.");
        }

        logger->info("bye");
    }

    class DependencyRegisterer {
    private:
        std::shared_ptr<JuceApp> app;
    public:
        explicit DependencyRegisterer(std::shared_ptr<JuceApp> parentApp) : app(std::move(parentApp)) {}

        void theme() {
            auto themeFilePath = PathFinder::liveTheme();
            if (!themeFilePath) {
                logger->error("Failed to get theme file path");
                return;
            }

            app->container_.registerFactory<Theme>(
                [themeFilePath](DependencyContainer&) { return std::make_shared<Theme>(*themeFilePath); }
                , DependencyContainer::Lifetime::Singleton
            );

            app->container_.registerFactory<LimLookAndFeel>(
                [](DependencyContainer& c) -> std::shared_ptr<LimLookAndFeel> {
                    return std::make_shared<LimLookAndFeel>(
                        [&c]() { return c.resolve<Theme>(); }
                    );
                }
                , DependencyContainer::Lifetime::Singleton
            );
        }

        void configFiles() {
            auto configFilePath = PathFinder::config();
            auto configMenuPath = PathFinder::configMenu();

            if (!configFilePath || !configMenuPath) {
                logger->error("Failed to get config file paths");
                return;
            }

            app->container_.registerFactory<ConfigManager>(
                [configFilePath](DependencyContainer&) { return std::make_shared<ConfigManager>(*configFilePath); }
                , DependencyContainer::Lifetime::Singleton
            );

            app->container_.registerFactory<ConfigMenu>(
                [configMenuPath](DependencyContainer&) { return std::make_shared<ConfigMenu>(*configMenuPath); }
                , DependencyContainer::Lifetime::Singleton
            );
        }

        void eventHandler() {
            app->container_.registerFactory<IEventHandler>(
                [](DependencyContainer& c) -> std::shared_ptr<IEventHandler> {
                    // We can delay these resolutions if needed
                    return std::make_shared<EventHandler>(
                        [&c]() { return c.resolve<IActionHandler>(); }
                        , [&c]() { return c.resolve<WindowManager>(); }
                    );
                }
                , DependencyContainer::Lifetime::Singleton
            );
        }

        void liveInterfaceAndStartObservers() {
            app->container_.registerFactory<ILiveInterface>(
                [](DependencyContainer& c) -> std::shared_ptr<ILiveInterface> {
                    // We can delay these resolutions if needed
                    return std::make_shared<LiveInterface>(
                        [&c]() { return c.resolve<IEventHandler>(); }
                    );
                }
                , DependencyContainer::Lifetime::Singleton
            );
            // kick off the window observers
            app->container_.resolve<ILiveInterface>();
        }

        void responseParser() {
            app->container_.registerFactory<ResponseParser>(
                [](DependencyContainer&) { return std::make_shared<ResponseParser>(); }
            );
        }

        void keySender() {
            app->container_.registerFactory<KeySender>(
                [](DependencyContainer&) { return std::make_shared<KeySender>(); }
                , DependencyContainer::Lifetime::Singleton
            );
        }

        void ipc() {
            app->container_.registerFactory<IIPCCore>(
                [](DependencyContainer& c) -> std::shared_ptr<IIPCCore> {
                    return std::make_shared<IPCResilienceDecorator>(
                        []() {
                            return std::make_shared<IPCCore>();
                        }
                    );
                }
                , DependencyContainer::Lifetime::Singleton
            );
        }

        void pluginManager() {
            app->container_.registerFactory<IPluginManager>(
                [](DependencyContainer& c) -> std::shared_ptr<PluginManager> {
                    return std::make_shared<PluginManager>(
                        [&c]() { return c.resolve<IIPCCore>(); }
                        , [&c]() { return c.resolve<ResponseParser>(); }
                    );
                }
                , DependencyContainer::Lifetime::Singleton
            );
        }

        void actionHandler() {
            app->container_.registerFactory<IActionHandler>(
                [](DependencyContainer& c) -> std::shared_ptr<IActionHandler> {
                    // We can delay these resolutions if needed
                    return std::make_shared<ActionHandler>(
                        [&c]() { return c.resolve<IPluginManager>(); }
                        , [&c]() { return c.resolve<WindowManager>(); }
                        , [&c]() { return c.resolve<ConfigManager>(); }
                        , [&c]() { return c.resolve<IIPCCore>(); }
                        , [&c]() { return c.resolve<IEventHandler>(); }
                        , [&c]() { return c.resolve<ILiveInterface>(); }
                    );
                }
                , DependencyContainer::Lifetime::Singleton
            );
        }

        void windowManager() {
            app->container_.registerFactory<WindowManager>(
                [](DependencyContainer& c) -> std::shared_ptr<WindowManager> {
                    // We can delay these resolutions if needed
                    return std::make_shared<WindowManager>(
                        [&c]() { return c.resolve<IPluginManager>(); }
                        , [&c]() { return c.resolve<IEventHandler>(); }
                        , [&c]() { return c.resolve<IActionHandler>(); }
                        , [&c]() { return c.resolve<WindowManager>(); }
                        , [&c]() { return c.resolve<Theme>(); }
                        , [&c]() { return c.resolve<LimLookAndFeel>(); }
                        , [&c]() { return c.resolve<ConfigMenu>(); }
                    );
                }
                , DependencyContainer::Lifetime::Singleton
            );
        }
    };
};

// NOLINTBEGIN
START_JUCE_APPLICATION(JuceApp)
// NOLINTEND
