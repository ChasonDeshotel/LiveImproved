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

    bool init();
    
    bool writeRequest(const std::string& message);
    std::string readResponse();

private:
    ApplicationManager& app_;
    LogHandler* log_;

    std::string requestPipePath = "/Users/cdeshotel/Scripts/Ableton/LiveImproved/request";
    std::string responsePipePath = "/Users/cdeshotel/Scripts/Ableton/LiveImproved/response";

    std::map<std::string, int> pipes_;

    bool createPipe(const std::string& pipe_name);
    bool openPipeForWrite(const std::string& pipe_name, bool non_blocking = false);
    bool openPipeForRead(const std::string& pipe_name, bool non_blocking = false);

    bool writeToPipe(const std::string& pipe_name, const std::string& message);
    std::string readFromPipe(const std::string& pipe_name);
};

#endif 
