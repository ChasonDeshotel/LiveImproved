#ifndef IPC_H
#define IPC_H

#include <iostream>
#include <map>
#include <string>
#include <windows.h>
#include <functional>

class ApplicationManager;
class LogHandler;

class IPC {
public:
    IPC(ApplicationManager& appManager);
    ~IPC();

    bool init();

    bool writeRequest(const std::string& message);
    std::string readResponse();
    bool initReadWithEventLoop(std::function<void(const std::string&)> callback);
    void drainPipe(HANDLE pipe);

private:
    ApplicationManager& app_;
    LogHandler* log_;

    std::string readResponseInternal(HANDLE pipe);

    std::string requestPipePath = "\\\\.\\pipe\\lim_request";
    std::string responsePipePath = "\\\\.\\pipe\\lim_response";

    std::map<std::string, HANDLE> pipes_;

    void removePipeIfExists(const std::string& pipe_name);

    bool createPipe(const std::string& pipe_name);
    bool openPipeForWrite(const std::string& pipe_name);
    bool openPipeForRead(const std::string& pipe_name);

    bool writeToPipe(HANDLE pipe, const std::string& message);
    std::string readFromPipe(HANDLE pipe);
};

#endif