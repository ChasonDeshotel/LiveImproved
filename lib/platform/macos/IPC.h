#ifndef IPC_H
#define IPC_H

#include <iostream>
#include <map>
#include <string>

class ApplicationManager;
class LogHandler;

class IPC {
public:
    IPC(ApplicationManager& appManager);
    ~IPC();

    void init();

    void createPipe(const std::string& pipe_name);
    void openPipeForWrite(const std::string& pipe_name, bool non_blocking = false);
    void openPipeForRead(const std::string& pipe_name, bool non_blocking = false);

    void writeToPipe(const std::string& pipe_name, const std::string& message);
    std::string readFromPipe(const std::string& pipe_name);

private:
    ApplicationManager& app_;
    LogHandler* log_;

    std::map<std::string, int> pipes_;
};

#endif 
