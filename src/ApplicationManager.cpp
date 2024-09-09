#include <string>
#include <cstdlib>
#include <filesystem>

#include <QApplication>

#include "ApplicationManager.h"
#include "LogHandler.h"
#include "PlatformDependent.h"
#include "ActionHandler.h"
#include "ResponseParser.h"
#include "ConfigManager.h"
#include "ConfigMenu.h"

ApplicationManager::ApplicationManager()
    : logHandler_(&LogHandler::getInstance())
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

int main(int argc, char *argv[]) {
    ApplicationManager& appManager = ApplicationManager::getInstance();

    appManager.getLogHandler()->info("Application started");
    appManager.getLogHandler()->info("int main()");

    static QApplication app(argc, argv);

    appManager.init();
    appManager.getLogHandler()->info("ApplicatonManager::init() called");

    // block until Live is running
    PID::getInstance().livePIDBlocking();

    PlatformInitializer::init();

    appManager.getEventHandler()->setupQuartzEventTap();

    PlatformInitializer::run();
}

#endif

void ApplicationManager::init() {
    logHandler_->info("ApplicatonManager::init() called");

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
    responseParser_ = new ResponseParser(*this);
    actionHandler_  = new ActionHandler(*this);
    keySender_      = new KeySender(*this);

    guiSearchBox_   = new GUISearchBox(*this);
    dragTarget_     = new DragTarget(*this);

    logHandler_->info("ApplicatonManager::init() finished");
}

LogHandler* ApplicationManager::getLogHandler() {
    return logHandler_;
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
