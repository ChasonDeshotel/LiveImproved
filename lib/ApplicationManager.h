#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include "IPCUtils.h"

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

private:
    ApplicationManager();

    // singleton so delete the copy constructor and assignment operator
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;
    
    IPCUtils ipc;
    pid_t abletonLivePID;
};

#endif
