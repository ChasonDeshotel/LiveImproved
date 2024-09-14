#include <string>
#include <cstdlib>
#include <filesystem>

#include "AppConfig.h"

#include "ApplicationManager.h"
#include "LogHandler.h"
#include "PlatformDependent.h"
#include "ActionHandler.h"
#include "ResponseParser.h"
#include "ConfigManager.h"
#include "ConfigMenu.h"
#include "PluginManager.h"
#include "KeySender.h"

ApplicationManager::ApplicationManager()
    : log_(&LogHandler::getInstance())
    , pluginManager_(nullptr)
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


#endif

void ApplicationManager::init() {
    log_->debug("ApplicatonManager::init() called");

    windowManager_  = new WindowManager();

    std::filesystem::path configFilePath =
        std::filesystem::path(std::string(getenv("HOME")))
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "config.txt"
    ;
    configManager_  = new ConfigManager(configFilePath);

    std::filesystem::path configMenuPath =
        std::filesystem::path(std::string(getenv("HOME")))
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "config-menu.txt"
    ;
    configMenu_     = new ConfigMenu(configMenuPath);

    eventHandler_   = new EventHandler(*this);

    ipc_            = new IPC(*this);

    pluginManager_ = new PluginManager(*ipc_, *responseParser_);
    pluginManager_->refreshPlugins();

    actionHandler_  = new ActionHandler(*this, *pluginManager_);
    KeySender::getInstance();

    responseParser_ = new ResponseParser();


    log_->debug("ApplicatonManager::init() finished");
}

LogHandler* ApplicationManager::getLogHandler() {
    return log_;
}

WindowManager* ApplicationManager::getWindowManager() {
    return windowManager_;
}

PluginManager& ApplicationManager::getPluginManager() {
    return *pluginManager_;
}

ConfigManager* ApplicationManager::getConfigManager() {
    return configManager_;
}

ConfigMenu* ApplicationManager::getConfigMenu() {
    return configMenu_;
}

IPC* ApplicationManager::getIPC() {
    return ipc_;
}

EventHandler* ApplicationManager::getEventHandler() {
    return eventHandler_;
}

ActionHandler* ApplicationManager::getActionHandler() {
    return actionHandler_;
}
