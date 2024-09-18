#ifndef IPC_H
#define IPC_H

#include <iostream>
#include <map>
#include <string>
#include <filesystem>

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
    void drainPipe(int fd);

private:
    ApplicationManager& app_;
    LogHandler* log_;

    std::string readResponseInternal(int fd);

    // TODO DRY
    std::filesystem::path getHomeDirectory() {
        #ifdef _WIN32
        const char* homeDir = getenv("USERPROFILE");
        #else
        const char* homeDir = getenv("HOME");
        #endif

        if (!homeDir) {
            throw std::runtime_error("Could not find the home directory.");
        }

        return std::filesystem::path(homeDir);
    }
    std::filesystem::path requestPipeFilePath =
        std::filesystem::path(getHomeDirectory())
        / "Documents" / "Ableton" "User Library"
        / "Remote Scripts" / "LiveImproved" / "lim_request"
    ;
    std::filesystem::path responsePipeFilePath =
        std::filesystem::path(getHomeDirectory())
        / "Documents" / "Ableton" "User Library"
        / "Remote Scripts" / "LiveImproved" / "lim_response"
    ;

    std::string requestPipePath = requestPipeFilePath.generic_string();
    std::string responsePipePath = responsePipeFilePath.generic_string();

    std::map<std::string, int> pipes_;

    void removePipeIfExists(const std::string& pipe_name);

    bool createPipe(const std::string& pipe_name);
    bool openPipeForWrite(const std::string& pipe_name, bool non_blocking = false);
    bool openPipeForRead(const std::string& pipe_name, bool non_blocking = false);

    bool writeToPipe(const std::string& pipe_name, const std::string& message);
    std::string readFromPipe(const std::string& pipe_name);
};

#endif 
