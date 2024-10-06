#include <JuceHeader.h>
#include <dispatch/dispatch.h>
#include <unistd.h>
#include <future>

#include "LogHandler.h"
#include "Utils.h"

#include "DependencyContainer.h"

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "ConfigMenu.h"
#include "KeySender.h"
#include "LimLookAndFeel.h"
#include "PlatformDependent.h"
#include "PluginManager.h"
#include "ResponseParser.h"
#include "Theme.h"
#include "WindowManager.h"

class JuceApp : public juce::JUCEApplication {
public:
    JuceApp()
        : container_(DependencyContainer::getInstance())
    {}

    const juce::String getApplicationName() override {
        return "Live Improved";
    }

    const juce::String getApplicationVersion() override {
        return "0.69.420";
    }

    void restartApplication() {
        juce::String executablePath = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName();

        // Spawn a new process to re-launch the application
        juce::ChildProcess process;
        if (process.start(executablePath)) {
            juce::Logger::writeToLog("Restarting application...");

            // delay so the new process can spawn. 1000 was not enough
            juce::Thread::sleep(5000);
        } else {
            juce::Logger::writeToLog("Failed to start new process");
        }
    }

    void initialise(const juce::String& commandLineArgs = "") override {
        std::locale::global(std::locale("en_US.UTF-8"));
        LogHandler::getInstance().info("Ignition sequence started...");

        juce::LookAndFeel::setDefaultLookAndFeel(limLookAndFeel_.get());

        container_.registerFactory<EventHandler>(
            [](DependencyContainer& c) -> std::shared_ptr<EventHandler> {
                // We can delay these resolutions if needed
                return std::make_shared<EventHandler>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                    , [&c]() { return c.resolve<IActionHandler>(); }
                    , [&c]() { return c.resolve<WindowManager>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.resolve<EventHandler>()->registerAppTermination([this]() {
            LogHandler::getInstance().info("termination callback called");

            auto ipc = this->container_.resolve<IIPC>();
            ipc->stopIPC();

            std::promise<void> closePromise;
            std::future<void> closeFuture = closePromise.get_future();

            std::thread([&closePromise, ipc, this]() {
                ipc->closeAndDeletePipes();
                closePromise.set_value();  // Notify that pipes are closed
            }).detach();
            closeFuture.wait();

            LogHandler::getInstance().info("restarting");
            restartApplication();
        });

        // TODO needs a isRunning for multiple instances?
        if (PID::getInstance().livePID() == -1) {
            container_.resolve<EventHandler>()->registerAppLaunch([this]() {
                LogHandler::getInstance().info("launch callback called");
                // delay to let Live fully start up
                sleep(10);
                this->onLiveLaunch(0);
            });
        } else {
            onLiveLaunch(5);
        }
    }

    void onLiveLaunch(int ipcCallDelay) {
        // TODO cheap file exists checks
        std::filesystem::path configFilePath =
            std::filesystem::path(Utils::getHomeDirectory())
            / "Documents" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "config.txt"
        ;

        std::filesystem::path configMenuPath =
            std::filesystem::path(Utils::getHomeDirectory())
            / "Documents" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "config-menu.txt"
        ;

        std::filesystem::path themeFilePath =
            std::filesystem::path("/") / "Applications" / "Ableton Live 12 Suite.app"
            / "Contents" / "App-Resources" / "Themes" / "Default Dark Neutral High.ask"
        ;

        container_.registerFactory<ILogHandler>(
            // TODO maybe
            //[](DependencyContainer&) { return std::make_shared<LogHandler>("app.log"); },
            [](DependencyContainer&) { return std::make_shared<LogHandler>(); }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<Theme>(
            [themeFilePath](DependencyContainer&) { return std::make_shared<Theme>(themeFilePath); }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<LimLookAndFeel>(
            [](DependencyContainer& c) -> std::shared_ptr<LimLookAndFeel> {
                return std::make_shared<LimLookAndFeel>(
                    [&c]() { return c.resolve<Theme>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<ConfigManager>(
            [configFilePath](DependencyContainer&) { return std::make_shared<ConfigManager>(configFilePath); }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<ConfigMenu>(
            [configMenuPath](DependencyContainer&) { return std::make_shared<ConfigMenu>(configMenuPath); }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<ResponseParser>(
            [](DependencyContainer&) { return std::make_shared<ResponseParser>(); }
        );

        container_.registerFactory<KeySender>(
            [](DependencyContainer&) { return std::make_shared<KeySender>(); }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<IIPC>(
            [](DependencyContainer& c) -> std::shared_ptr<IPC> {
                // We can delay these resolutions if needed
                return std::make_shared<IPC>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<IPluginManager>(
            [](DependencyContainer& c) -> std::shared_ptr<IPluginManager> {
                return std::make_shared<PluginManager>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                    , [&c]() { return c.resolve<IIPC>(); }
                    , [&c]() { return c.resolve<ResponseParser>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<IActionHandler>(
            [](DependencyContainer& c) -> std::shared_ptr<ActionHandler> {
                // We can delay these resolutions if needed
                return std::make_shared<ActionHandler>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                    , [&c]() { return c.resolve<IPluginManager>(); }
                    , [&c]() { return c.resolve<WindowManager>(); }
                    , [&c]() { return c.resolve<ConfigManager>(); }
                    , [&c]() { return c.resolve<IIPC>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.registerFactory<WindowManager>(
            [](DependencyContainer& c) -> std::shared_ptr<WindowManager> {
                // We can delay these resolutions if needed
                return std::make_shared<WindowManager>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                    , [&c]() { return c.resolve<IPluginManager>(); }
                    , [&c]() { return c.resolve<EventHandler>(); }
                    , [&c]() { return c.resolve<IActionHandler>(); }
                    , [&c]() { return c.resolve<WindowManager>(); }
                    , [&c]() { return c.resolve<Theme>(); }
                    , [&c]() { return c.resolve<LimLookAndFeel>(); }
                    , [&c]() { return c.resolve<ConfigMenu>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container_.resolve<IIPC>()->init();

        if (ipcCallDelay > 0) sleep(ipcCallDelay);

        LogHandler::getInstance().info("writing READY");
        container_.resolve<IIPC>()->writeRequest("READY", [this](const std::string& response) {
            LogHandler::getInstance().info("received READY response: " + response);
        });

        // TODO: IPC queue should be able to handle more writes without sleep
        sleep(2);
        LogHandler::getInstance().info("refreshing plugin cache");
        container_.resolve<IPluginManager>()->refreshPlugins();

        #ifndef _WIN32
			PlatformInitializer::init();
			container_.resolve<EventHandler>()->setupQuartzEventTap();
			PlatformInitializer::run();
        #endif
    }

    void shutdown() override {
        auto ipc = this->container_.resolve<IIPC>();
        ipc->stopIPC();

        std::promise<void> closePromise;
        std::future<void> closeFuture = closePromise.get_future();

        std::thread([&closePromise, ipc, this]() {
            ipc->closeAndDeletePipes();
            closePromise.set_value();  // Notify that pipes are closed
        }).detach();
        closeFuture.wait();

        LogHandler::getInstance().info("bye");
    }

private:
    DependencyContainer& container_;
    std::unique_ptr<LimLookAndFeel> limLookAndFeel_;
};

START_JUCE_APPLICATION(JuceApp)
