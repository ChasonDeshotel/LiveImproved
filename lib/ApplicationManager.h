#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

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

private:
    // Private constructor to prevent direct instantiation
    ApplicationManager();

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

    PID* pid_ = nullptr;
    EventHandler* eventHandler_ = nullptr;
    ActionHandler* actionHandler_ = nullptr;
    KeySender* keySender_ = nullptr;

    LogHandler* logHandler_;
};

#endif
