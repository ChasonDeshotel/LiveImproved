#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include <vector>
#include <numeric>

#include "PlatformDependent.h"
#include "ActionHandler.h"
#include "LogHandler.h"

class EventHandler;

class ApplicationManager {
public:
    static ApplicationManager& getInstance() {
        static ApplicationManager instance;
        return instance;
    }

    void init();

    LogHandler* getLogHandler();

    pid_t getPID();
    EventHandler* getEventHandler();
    ActionHandler* getActionHandler();
    KeySender* getKeySender();
    IPC* getIPC();
    GUISearchBox* getGUISearchBox();

    std::vector<std::string> pluginCache;
    std::string getPluginCacheAsStr() const;

    void refreshPluginCache();

private:
    // Private constructor to prevent direct instantiation
    ApplicationManager();

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

    PID* pid_ = nullptr;
    IPC* ipc_ = nullptr;
    EventHandler* eventHandler_ = nullptr;
    ActionHandler* actionHandler_ = nullptr;
    KeySender* keySender_ = nullptr;
    GUISearchBox* guiSearchBox_ = nullptr;

    LogHandler* logHandler_;

    std::vector<std::string> splitStringInPlace(std::string& str, char delimiter);

};

#endif
