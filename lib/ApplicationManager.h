#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include "ApplicationManager.h"
#include "PlatformDependent.h"

class ApplicationManager {
public:
    static ApplicationManager& getInstance() {
        static ApplicationManager instance;
        return instance;
    }

    void initialize(EventHandler& eventHandler, KeySender& keySender);
    //void initialize(EventHandler* eventHandler, IPCManager* ipcManager, KeySender* keySender);

    //void run();
    //void sendMessage(const std::string& message);
    //std::string receiveMessage();

    // not cross platform compatible
    // move to platform/macos/init
    //pid_t getAbletonLivePID() const;
    //void setAbletonLivePID(pid_t pid);

//    ActionHandler& getActionHandler();
    EventHandler& getEventHandler();
//    IPCManager& getIPCManager();
    KeySender& getKeySender();

    // crashes
    //LogHandler& getLogHandler();

private:
    // Private constructor to prevent direct instantiation
    ApplicationManager() = default;

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

    EventHandler* eventHandler_ = nullptr;
    KeySender* keySender_ = nullptr;  // Pointer to hold the reference
    
    //ActionHandler* actionHandler;
    //IPCManager* ipc;
//    EventHandler* eventHandler_ = nullptr;
//    IPCManager* ipcManager_ = nullptr;

    // not cross platform compatible
    pid_t abletonLivePID;
};

#endif
