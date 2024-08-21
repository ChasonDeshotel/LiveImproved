#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include "ActionHandler.h"
#include "ApplicationManager.h"
#include "PlatformSpecific.h"

class ApplicationManager {
public:
    static ApplicationManager& getInstance() {
        static ApplicationManager instance;
        return instance;
    }

    void initialize();
    void run();
    void sendMessage(const std::string& message);
    std::string receiveMessage();

    // not cross platform compatible
    // move to platform/macos/init
    pid_t getAbletonLivePID() const;
    void setAbletonLivePID(pid_t pid);

    ActionHandler& getActionHandler();
    EventHandler& getEventHandler();
    IPCManager& getIPCManager();

    // crashes
    //LogHandler& getLogHandler();

private:
    ApplicationManager();

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;
    
    ActionHandler actionHandler;
    EventHandler eventHandler;
    IPCManager ipc;
    //LogHandler logHandler;

    KeySender keySender;

    // not cross platform compatible
    pid_t abletonLivePID;
};

#endif
