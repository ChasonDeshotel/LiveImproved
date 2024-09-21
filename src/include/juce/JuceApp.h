#include <JuceHeader.h>
#include <dispatch/dispatch.h>
#include "LogHandler.h"
#include "DependencyContainer.h"

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "ConfigMenu.h"
#include "KeySender.h"
#include "PlatformDependent.h"
#include "PluginManager.h"
#include "ResponseParser.h"
#include "WindowManager.h"

// TODO DRY violation
inline std::filesystem::path getHomeDirectory() {
    #ifdef _WIN32
		const char* homeDir = getenv("USERPROFILE");
    #else
		const char* homeDir = getenv("HOME");
    #endif

    if (!homeDir) {
        throw std::runtime_error("Could not find the home directory.");
    }

    return std::filesystem::path(homeDir);
}

class JuceApp : public juce::JUCEApplication {
public:
    JuceApp() {}

    const juce::String getApplicationName() override {
        return "Live Improved";
    }

    const juce::String getApplicationVersion() override {
        return "0.69.420";
    }

    void initialise(const juce::String&) override {
        std::locale::global(std::locale("en_US.UTF-8"));
        LogHandler::getInstance().info("Ignition sequence started...");

        // TODO cheap file exists checks
        std::filesystem::path configFilePath =
            std::filesystem::path(getHomeDirectory())
            / "Documents" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "config.txt"
        ;

        std::filesystem::path configMenuPath =
            std::filesystem::path(getHomeDirectory())
            / "Documents" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "config-menu.txt"
        ;

        // TODO use events/accessibility to see if Live is already open
        // and skip the delay
        PID::getInstance().livePIDBlocking();

        DependencyContainer& container = DependencyContainer::getInstance();

        container.registerFactory<ILogHandler>(
            // TODO maybe
            //[](DependencyContainer&) { return std::make_shared<LogHandler>("app.log"); },
            [](DependencyContainer&) { return std::make_shared<LogHandler>(); }
            , DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<ConfigManager>(
            [configFilePath](DependencyContainer&) { return std::make_shared<ConfigManager>(configFilePath); }
            , DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<ConfigMenu>(
            [configMenuPath](DependencyContainer&) { return std::make_shared<ConfigMenu>(configMenuPath); }
            , DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<ResponseParser>(
            [](DependencyContainer&) { return std::make_shared<ResponseParser>(); }
        );

        container.registerFactory<KeySender>(
            [](DependencyContainer&) { return std::make_shared<KeySender>(); }
            , DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<IIPC>(
            [](DependencyContainer& c) -> std::shared_ptr<IPC> {
                // We can delay these resolutions if needed
                return std::make_shared<IPC>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<IPluginManager>(
            [](DependencyContainer& c) -> std::shared_ptr<IPluginManager> {
                return std::make_shared<PluginManager>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                    , [&c]() { return c.resolve<IIPC>(); }
                    , [&c]() { return c.resolve<ResponseParser>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<EventHandler>(
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

        container.registerFactory<IActionHandler>(
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

        container.registerFactory<WindowManager>(
            [](DependencyContainer& c) -> std::shared_ptr<WindowManager> {
                // We can delay these resolutions if needed
                return std::make_shared<WindowManager>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                    , [&c]() { return c.resolve<IPluginManager>(); }
                    , [&c]() { return c.resolve<EventHandler>(); }
                    , [&c]() { return c.resolve<IActionHandler>(); }
                    , [&c]() { return c.resolve<WindowManager>(); }
                );
            }
            , DependencyContainer::Lifetime::Singleton
        );

        container.resolve<IIPC>()->init();
        LogHandler::getInstance().info("IPC read/write enabled");

        // TODO plugin refresh as callback from async init
        dispatch_queue_t backgroundQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_time_t delay;

        delay = dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC);
        dispatch_after(delay, backgroundQueue, ^{
            LogHandler::getInstance().info("writing READY");
            container.resolve<IIPC>()->writeRequest("READY", [this](const std::string& response) {
				LogHandler::getInstance().info("received READY response: " + response);
			});
        });

        delay = dispatch_time(DISPATCH_TIME_NOW, 4 * NSEC_PER_SEC);
        dispatch_after(delay, backgroundQueue, ^{
            LogHandler::getInstance().info("refreshing plugin cache");
            container.resolve<IPluginManager>()->refreshPlugins();
        });

        #ifndef _WIN32
			PlatformInitializer::init();
			container.resolve<EventHandler>()->setupQuartzEventTap();
			PlatformInitializer::run();
        #endif
    }

    void shutdown() override {
        // Clean up
        // TODO delete file pipes
    }
};
