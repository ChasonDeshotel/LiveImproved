#include <string>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>


#include "AppConfig.h"

#include "ApplicationManager.h"
#include "LogHandler.h"

#include "DependencyContainer.h"

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "ConfigMenu.h"
#include "KeySender.h"
#include "PlatformDependent.h"
#include "PluginManager.h"
#include "ResponseParser.h"
//#include "WindowManager.h"
#include "IIPC.h"
#include "IPluginManager.h"
#include "ILogHandler.h"

ApplicationManager::ApplicationManager()
    : container_(std::make_unique<DependencyContainer>())
{}

#ifdef INJECTED_LIBRARY

__attribute__((constructor))
static void dylib_init() {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        LogHandler::getInstance().info("injected successfully");

        ApplicationManager& appManager = ApplicationManager::getInstance();
        appManager.init();
    });
}

#else

// int main used to go here

#endif

//std::filesystem::path getHomeDirectory() {
//    #ifdef _WIN32
//		const char* homeDir = getenv("USERPROFILE");
//    #else
//		const char* homeDir = getenv("HOME");
//    #endif
//
//    if (!homeDir) {
//        throw std::runtime_error("Could not find the home directory.");
//    }
//
//    return std::filesystem::path(homeDir);
//}

void ApplicationManager::init() {
//    log_->debug("ApplicatonManager::init() called");

//    std::filesystem::path configFilePath =
//        std::filesystem::path(getHomeDirectory())
//        / "Documents" / "Ableton" / "User Library"
//        / "Remote Scripts" / "LiveImproved" / "config.txt"
//    ;
//    configManager_  = new ConfigManager(configFilePath);

    //std::filesystem::path configMenuPath =
    //    std::filesystem::path(getHomeDirectory())
    //    / "Documents" / "Ableton" / "User Library"
    //    / "Remote Scripts" / "LiveImproved" / "config-menu.txt"
    //;
    //configMenu_     = new ConfigMenu(configMenuPath);

//    ipc_            = new IPC(*this);

//    responseParser_ = new ResponseParser();


    if (!container_) {
        throw std::runtime_error("DependencyContainer not initialized");
    }

    container_->registerSingleton<ILogHandler, LogHandler>();
    container_->registerType<ResponseParser, ResponseParser>();
    container_->registerType<IIPC, IPC>(
        container_->resolve<ILogHandler>()
        , std::weak_ptr<IPluginManager>(container_->resolve<IPluginManager>())
    );

    container_->registerType<IPluginManager, PluginManager>(
        container_->resolve<ILogHandler>()
        , std::weak_ptr<IIPC>(container_->resolve<IIPC>())
        , container_->resolve<ResponseParser>()
    );
//
//    container_->registerType<ResponseParser>([]() {
//        return std::make_shared<ResponseParser>();
//    });
//
//    container_->registerType<IActionHandler>([&, this]() {
//        return std::make_shared<ActionHandler>(
//            container_->resolve<ILogHandler>()
//            , container_->resolve<IPluginManager>()
//            , container_->resolve<WindowManager>()
//            , container_->resolve<ConfigManager>()
//            , container_->resolve<IIPC>()
//        );
//    });

//    KeySender::getInstance();

    // TODO add initialized flag on PluginManager and block until we have plugins
//    eventHandler_   = new EventHandler(*windowManager_, *actionHandler_);

//    log_->debug("ApplicatonManager::init() finished");
}
