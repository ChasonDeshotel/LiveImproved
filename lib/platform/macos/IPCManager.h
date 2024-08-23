#ifndef IPCMANAGER_H
#define IPCMANAGER_H

#include <iostream>
#include <map>
#include <string>

class ApplicationManager;
class LogHandler;

class IPCManager {
public:
    IPCManager(ApplicationManager& appManager);
    ~IPCManager();

    void init();

    void createPipe(const std::string& pipe_name);
    void openPipeForWrite(const std::string& pipe_name, bool non_blocking = false);
    void openPipeForRead(const std::string& pipe_name, bool non_blocking = false);

    void writeToPipe(const std::string& pipe_name, const std::string& message);
    std::string readFromPipe(const std::string& pipe_name);

private:
    std::map<std::string, int> pipes_;
};

#endif 
