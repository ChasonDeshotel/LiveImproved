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
#include "KeySender.h"

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
    actionHandler_  = new ActionHandler(*this);
    KeySender::getInstance();

    ipc_            = new IPC(*this);
    responseParser_ = new ResponseParser(*this);

    log_->debug("ApplicatonManager::init() finished");
}

LogHandler* ApplicationManager::getLogHandler() {
    return log_;
}

WindowManager* ApplicationManager::getWindowManager() {
    return windowManager_;
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

const std::vector<Plugin>& ApplicationManager::getPlugins() const {
    return plugins_;
}

void ApplicationManager::refreshPluginCache() {
    ipc_->writeRequest("PLUGINS");
    // Set up a callback to handle the response asynchronously using dispatch
    ipc_->initReadWithEventLoop([this](const std::string& response) {
        if (!response.empty()) {
            //LogHandler::getInstance().debug("Received response: " + response);
            pluginCacheStr = response;
            plugins_ = responseParser_->parsePlugins(response);
        } else {
            LogHandler::getInstance().error("Failed to receive a valid response.");
        }
    });
}
