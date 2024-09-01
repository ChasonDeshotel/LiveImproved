#include <string>
#include <cstdlib>
#include <filesystem>

#include "ApplicationManager.h"
#include "LogHandler.h"
#include "PlatformDependent.h"
#include "ActionHandler.h"
#include "ResponseParser.h"
#include "ConfigManager.h"

ApplicationManager::ApplicationManager()
    : logHandler_(&LogHandler::getInstance())
{
}

void ApplicationManager::init() {
    logHandler_->info("ApplicatonManager::init() called");

    std::filesystem::path configFilePath =
        std::filesystem::path(std::string(getenv("HOME")))
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "config.txt"
    ;
    configManager_ = new ConfigManager(configFilePath);

    // crashed when chaining
    eventHandler_ = new EventHandler(*this);
    eventHandler_->init(); // start event loop

    ipc_ = new IPC(*this);
    ipc_->init();

    responseParser_ = new ResponseParser(*this);

    actionHandler_ = new ActionHandler(*this);
    keySender_ = new KeySender(*this);

    guiSearchBox_ = new GUISearchBox(*this);
    dragTarget_ = new DragTarget(*this);

    logHandler_->info("ApplicatonManager::init() finished");
}

LogHandler* ApplicationManager::getLogHandler() {
    return logHandler_;
}

ConfigManager* ApplicationManager::getConfigManager() {
    return configManager_;
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

KeySender* ApplicationManager::getKeySender() {
    return keySender_;
}

GUISearchBox* ApplicationManager::getGUISearchBox() {
    return guiSearchBox_;
}

DragTarget* ApplicationManager::getDragTarget() {
    return dragTarget_;
}

std::vector<Plugin> ApplicationManager::getPlugins() {
    return plugins_;
}

void ApplicationManager::refreshPluginCache() {
    ipc_->writeRequest("PLUGINS");
    // Set up a callback to handle the response asynchronously using dispatch
    ipc_->initReadWithEventLoop([this](const std::string& response) {
        if (!response.empty()) {
        LogHandler::getInstance().info("Received response: " + response);
            pluginCacheStr = response;
            plugins_ = responseParser_->parsePlugins(response);

            guiSearchBox_->setOptions(plugins_);
        } else {
            LogHandler::getInstance().info("Failed to receive a valid response.");
        }
    });
}

//std::string ApplicationManager::getPluginsAsStr() const {
//    if (pluginCache.empty()) {
//        return "";
//    }
//
//    return std::accumulate(std::next(plugins_.begin()), plugins_.end(), plugins_[0],
//        [](const std::string& a, const std::string& b) { return a + ',' + b; });
//}
