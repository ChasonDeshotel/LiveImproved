#ifndef TEST_BUILD
#include <JuceHeader.h>
#endif
#include <utility>

#include "LogGlobal.h"
#include "PathFinder.h"

#include "IEventHandler.h"
#include "IIPCCore.h"
#include "ILiveInterface.h"
#include "IPCCore.h"

#include "DependencyContainer.h"
#include "PlatformInitializer.h"

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
        for (auto pid : PID::getInstance().findLiveImproveds()) {
            if (pid != PID::getInstance().appPID()) {
                logger->warn("Already running LiveImproved instance with PID {} will to now be kill.", pid);
                kill(pid, SIGTERM);
            }
        }
        juce::Thread::sleep(500);

        for (auto pid : PID::getInstance().findLiveImproveds()) {
            if (pid != PID::getInstance().appPID()) {
                logger->warn("Stubborn LiveImproved instance with PID {} now is get killed but more.", pid);
                kill(pid, SIGKILL);
            }
        }

        //
        // TODO add check for is LES running
        //
        std::locale::global(std::locale("en_US.UTF-8"));

        logger->info("Ignition sequence started...");

        juce::LookAndFeel::setDefaultLookAndFeel(limLookAndFeel_.get());

        onLiveLaunch(DEFAULT_IPC_DELAY);
    }

    void onLiveLaunch(int ipcCallDelay) {
        logger->info("onLiveLaunch() called");

        DependencyRegisterer r(std::make_shared<JuceApp>());
        r.configFiles();
        r.theme();
        r.eventHandler();
        r.liveInterfaceAndStartObservers();
        r.responseParser();
        r.keySender();
        r.ipc();
        r.pluginManager();
        r.actionHandler();
        r.windowManager();

        if (ipcCallDelay > 0) juce::Thread::sleep(ipcCallDelay);

        auto ipc = container_.resolve<IIPCCore>();
        ipc->init();

        while (!ipc->isInitialized()) {
            logger->info("waiting for ipc init");
            juce::Thread::sleep(100);
        }

        logger->info("refreshing plugin cache");
        container_.resolve<IPluginManager>()->refreshPlugins();

        #ifndef _WIN32
        PlatformInitializer::init();
        container_.resolve<IEventHandler>()->setupQuartzEventTap();
        PlatformInitializer::run();
        #endif
    }

    void shutdown() override {
        try {
            auto ipc = this->container_.resolve<IIPCCore>();
            if (ipc) {
                logger->info("stopping IPC...");
                ipc->stopIPC();
                ipc->destroy();
            }
        } catch (const std::exception& e) {
            logger->error("Failed to resolve IIPCCore: {}", std::string(e.what()));
        } catch (...) {
            logger->error("Unknown error occurred while resolving IIPCCore.");
        }

        logger->info("Goodbye.");
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

        void liveInterfaceAndStartObservers() {
            app->container_.registerFactory<ILiveInterface>(
                [](DependencyContainer& c) -> std::shared_ptr<ILiveInterface> {
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
                    return std::make_shared<IPCCore>();
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

        void eventHandler() {
            app->container_.registerFactory<IEventHandler>(
                [](DependencyContainer& c) -> std::shared_ptr<IEventHandler> {
                    return std::make_shared<EventHandler>(
                        [&c]() { return c.resolve<IActionHandler>(); }
                        , [&c]() { return c.resolve<WindowManager>(); }
                    );
                }
                , DependencyContainer::Lifetime::Singleton
            );
        }

        void actionHandler() {
            app->container_.registerFactory<IActionHandler>(
                [](DependencyContainer& c) -> std::shared_ptr<IActionHandler> {
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

juce ::JUCEApplicationBase *juce_CreateApplication();
juce ::JUCEApplicationBase *juce_CreateApplication() { return new JuceApp(); } // NOLINT
int main(int argc, char *argv[]) {
    juce ::JUCEApplicationBase ::createInstance = &juce_CreateApplication;
    return juce ::JUCEApplicationBase ::main(argc, (const char **)argv);
}
