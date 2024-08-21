#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include "PlatformSpecific.h"
#include "ActionHandler.h"

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

private:
    ApplicationManager();

    // singleton so delete the copy constructor and assignment operator
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;
    
    ActionHandler actionHandler;
    EventHandler eventHandler;

    // should be singleton
    IPCManager ipc;

    KeySender keySender;

    // not cross platform compatible
    pid_t abletonLivePID;
};

#endif
