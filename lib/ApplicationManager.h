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

    //void initialize();
    void initialize();

    LogHandler& getLogHandler();

    EventHandler* getEventHandler();
    ActionHandler* getActionHandler();
    KeySender* getKeySender();

      //  actionHandler_ = new ActionHandler();
      //  keySender_ = new KeySender();

        // Additional initialization logic if needed

//    ActionHandler& getActionHandler();
//    KeySender& getKeySender();
        //EventHandler& eventHandler
//        , ActionHandler& actionHandler
//        , KeySender& keySender
    //void initialize(EventHandler* eventHandler, IPCManager* ipcManager, KeySender* keySender);

    //void run();
    //void sendMessage(const std::string& message);
    //std::string receiveMessage();

    // not cross platform compatible
    // move to platform/macos/init
    //pid_t getAbletonLivePID() const;
    //void setAbletonLivePID(pid_t pid);

//    ActionHandler& getActionHandler();
//    IPCManager& getIPCManager();
//    KeySender& getKeySender();

    // crashes
    //LogHandler& getLogHandler();

private:
    // Private constructor to prevent direct instantiation
    ApplicationManager();

    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

    EventHandler* eventHandler_ = nullptr;
    ActionHandler* actionHandler_ = nullptr;
    KeySender* keySender_ = nullptr;

    LogHandler& logHandler_;
//    ActionHandler* actionHandler_ = nullptr;
//    KeySender* keySender_ = nullptr;  // Pointer to hold the reference
    
    //ActionHandler* actionHandler;
    //IPCManager* ipc;
//    EventHandler* eventHandler_ = nullptr;
//    IPCManager* ipcManager_ = nullptr;

    // not cross platform compatible
    pid_t abletonLivePID;
};

#endif
