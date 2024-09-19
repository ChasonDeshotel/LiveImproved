#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include <cstdlib>
#include <vector>
#include <numeric>

#include "PlatformDependent.h"
#include "DependencyContainer.h"

class ActionHandler;
class ConfigManager;
class ConfigMenu;
class EventHandler;
class LogHandler;
class PluginManager;
class ResponseParser;
class WindowManager;
class IPC;

class ApplicationManager {
public:
    static ApplicationManager& getInstance() {
        static ApplicationManager instance;
        return instance;
    }

    void init();

private:
    ApplicationManager();
    std::unique_ptr<DependencyContainer> container_;

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

    LogHandler*     log_;

};

#endif
