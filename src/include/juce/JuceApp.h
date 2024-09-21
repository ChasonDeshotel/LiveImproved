#include <JuceHeader.h>
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
#include "IIPC.h"
#include "IPluginManager.h"
#include "ILogHandler.h"

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
        return "0.0.0.1";
    }

    void initialise(const juce::String&) override {
        std::locale::global(std::locale("en_US.UTF-8"));
        LogHandler::getInstance().info("Ignition sequence started...");

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
        
        // TODO cheap file exists checks

        PID::getInstance().livePIDBlocking();

//    configManager_  = new ConfigManager(configFilePath);

    //configMenu_     = new ConfigMenu(configMenuPath);

        DependencyContainer& container = DependencyContainer::getInstance();

        container.registerFactory<ILogHandler>(
            // TODO maybe
            //[](DependencyContainer&) { return std::make_shared<LogHandler>("app.log"); },
            [](DependencyContainer&) { return std::make_shared<LogHandler>(); },
            DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<ResponseParser>(
            [](DependencyContainer&) { return std::make_shared<ResponseParser>(); }
        );

        container.registerFactory<IIPC>(
            [](DependencyContainer& c) -> std::shared_ptr<IPC> {
                // We can delay these resolutions if needed
                return std::make_shared<IPC>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                );
            },
            DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<IPluginManager>(
            [](DependencyContainer& c) -> std::shared_ptr<IPluginManager> {
                return std::make_shared<PluginManager>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                    , [&c]() { return c.resolve<IIPC>(); }
                    , [&c]() { return c.resolve<ResponseParser>(); }
                );
            },
          DependencyContainer::Lifetime::Singleton
        );

        container.registerFactory<EventHandler>(
            [](DependencyContainer& c) -> std::shared_ptr<EventHandler> {
                // We can delay these resolutions if needed
                return std::make_shared<EventHandler>(
                    [&c]() { return c.resolve<ILogHandler>(); }
                    , [&c]() { return c.resolve<IActionHandler>(); }
                    , [&c]() { return c.resolve<WindowManager>(); }
                );
            },
          DependencyContainer::Lifetime::Singleton
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
            },
          DependencyContainer::Lifetime::Singleton
        );

        container.resolve<IIPC>()->init();

//        container.registerType<WindowManager, WindowManager, IPluginManager, EventHandler, IActionHandler, WindowManager>(DependencyContainer::Lifetime::Singleton);


    // timer to attempt opening the request pipe 
    // without log jamming bableton
//    dispatch_queue_t queue = dispatch_get_main_queue();
//    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
//    dispatch_source_set_timer(timer, DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC, 0);  // 100ms interval
//    dispatch_source_set_event_handler(timer, ^{
//        if (openPipeForWrite(requestPipePath, true)) {
//            log_->info("request pipe successfully opened for writing");
//
//            dispatch_queue_t backgroundQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
//            dispatch_time_t delay;
//            delay = dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC);
//
//            dispatch_after(delay, backgroundQueue, ^{
//                log_->info("writing READY");
//                // TODO check response
//                writeRequest("READY");
//            });
//
//            // account for Live startup delay
//            // TODO use events/accessibility to see if Live is already open
//            // and skip this delay
//            delay = dispatch_time(DISPATCH_TIME_NOW, 4 * NSEC_PER_SEC);
//            dispatch_after(delay, backgroundQueue, ^{
//                log_->info("refreshing plugin cache");
//                auto pluginManager = pluginManager_.lock();
//                if (pluginManager) {
//                    pluginManager->refreshPlugins();
//                } else {
//                    log_->error("PluginManager is no longer available");
//                }
//            });
//
//            dispatch_source_cancel(timer);
//        } else {
//            log_->error("Attempt to open request pipe for writing failed. Retrying...");
//        }
//    });
//    dispatch_resume(timer);

//        container->registerType<IIPC, IPC>(
//            container->resolve<ILogHandler>()
//            , std::weak_ptr<IPluginManager>(container->resolve<IPluginManager>())
//        );
//        // TODO TODO run IPC->init here
//
//        container->registerType<IPluginManager, PluginManager>(
//            container->resolve<ILogHandler>()
//            , std::weak_ptr<IIPC>(container->resolve<IIPC>())
//            , container->resolve<ResponseParser>()
//        );
//
//    container->registerType<ResponseParser>([]() {
//        return std::make_shared<ResponseParser>();
//    });
//
//    container->registerType<IActionHandler>([&, this]() {
//        return std::make_shared<ActionHandler>(
//            container->resolve<ILogHandler>()
//            , container->resolve<IPluginManager>()
//            , container->resolve<WindowManager>()
//            , container->resolve<ConfigManager>()
//            , container->resolve<IIPC>()
//        );
//    });

//    KeySender::getInstance();

    // TODO add initialized flag on PluginManager and block until we have plugins
//    eventHandler_   = new EventHandler(*windowManager_, *actionHandler_);

//    log_->debug("ApplicatonManager::init() finished");

        // Block until Live is running
        // file pipes act fucky on windows
        // Live doesn't boot first
        // TODO implement monitoring to see when
        // live opens and closes instead of looping
        
//        #ifndef _WIN32
//			PlatformInitializer::init();
//			appManager.getEventHandler()->setupQuartzEventTap();
//			PlatformInitializer::run();
//        #endif
    }

    void shutdown() override {
        // Clean up
        // TODO delete file pipes
    }
};
