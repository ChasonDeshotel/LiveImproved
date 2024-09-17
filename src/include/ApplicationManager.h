#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include <vector>
#include <numeric>

#include "PlatformDependent.h"

class ActionHandler;
class ConfigManager;
class ConfigMenu;
class EventHandler;
class LogHandler;
class PluginManager;
class ResponseParser;
class WindowManager;

class ApplicationManager {
public:
    static ApplicationManager& getInstance() {
        static ApplicationManager instance;
        return instance;
    }

    void init();

    ActionHandler*  getActionHandler();
    ConfigManager*  getConfigManager();
    ConfigMenu*     getConfigMenu();
    EventHandler*   getEventHandler();
    IPC*            getIPC();
    LogHandler*     getLogHandler();
    PluginManager&  getPluginManager();
    WindowManager*  getWindowManager();

private:
    ApplicationManager();

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

    ActionHandler*  actionHandler_  = nullptr;
    ConfigManager*  configManager_  = nullptr;
    ConfigMenu*     configMenu_     = nullptr;
    EventHandler*   eventHandler_   = nullptr;
    IPC*            ipc_            = nullptr;
    KeySender*      keySender_      = nullptr;
    PluginManager*  pluginManager_  = nullptr;
    ResponseParser* responseParser_ = nullptr;
    WindowManager*  windowManager_  = nullptr;

    LogHandler*     log_;

};

#endif
