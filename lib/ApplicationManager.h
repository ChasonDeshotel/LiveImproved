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

    pid_t getAbletonLivePID() const;
    void setAbletonLivePID(pid_t pid);

    ActionHandler& getActionHandler();
    EventHandler& getEventHandler();

private:
    ApplicationManager();

    // singleton so delete the copy constructor and assignment operator
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;
    
    ActionHandler& actionHandler;
    IPCUtils ipc;
    EventHandler eventHandler;
    KeySender keySender;
    pid_t abletonLivePID;
};

#endif
