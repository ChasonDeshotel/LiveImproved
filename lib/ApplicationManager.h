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

    void initialize();

    LogHandler& getLogHandler();

    EventHandler* getEventHandler();
    ActionHandler* getActionHandler();
    KeySender* getKeySender();

    // not cross platform compatible
    // move to platform/macos/init
    //pid_t getAbletonLivePID() const;
    //void setAbletonLivePID(pid_t pid);

private:
    // Private constructor to prevent direct instantiation
    ApplicationManager();

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

    EventHandler* eventHandler_ = nullptr;
    ActionHandler* actionHandler_ = nullptr;
    KeySender* keySender_ = nullptr;

    LogHandler& logHandler_;

    // not cross platform compatible
    pid_t abletonLivePID;
};

#endif
