#include <string>
#include <cstdlib>
#include <filesystem>

#include "AppConfig.h"

#include "ApplicationManager.h"
#include "LogHandler.h"

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "ConfigMenu.h"
#include "KeySender.h"
#include "PlatformDependent.h"
#include "PluginManager.h"
#include "ResponseParser.h"
#include "WindowManager.h"

ApplicationManager::ApplicationManager()
    : log_(&LogHandler::getInstance())
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

std::filesystem::path getHomeDirectory() {
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

void ApplicationManager::init() {
    log_->debug("ApplicatonManager::init() called");

    windowManager_  = new WindowManager();

    std::filesystem::path configFilePath =
        std::filesystem::path(getHomeDirectory())
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "config.txt"
    ;
    configManager_  = new ConfigManager(configFilePath);

    std::filesystem::path configMenuPath =
        std::filesystem::path(getHomeDirectory())
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "config-menu.txt"
    ;
    configMenu_     = new ConfigMenu(configMenuPath);

    ipc_            = new IPC(*this);

    responseParser_ = new ResponseParser();
    pluginManager_  = new PluginManager(*ipc_, *responseParser_);

    actionHandler_  = new ActionHandler(*ipc_, *pluginManager_, *windowManager_, *configManager_);
    KeySender::getInstance();

    // TODO add initialized flag on PluginManager and block until we have plugins
    eventHandler_   = new EventHandler(*windowManager_, *actionHandler_);

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
