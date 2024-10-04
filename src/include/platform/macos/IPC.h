#pragma once

#include <atomic>
#include <iostream>
#include <map>
#include <string>
#include <filesystem>
#include <thread>
#include <string>
#include <queue>

#include "Utils.h"
#include "IIPC.h"

class ApplicationManager;
class ILogHandler;
class IPluginManager;

class IPC : public IIPC {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    IPC(
        std::function<std::shared_ptr<ILogHandler>()> logHandler
       );
    ~IPC() override;

    bool init() override;
    
    void writeRequest(const std::string& message, ResponseCallback callback) override;
    void writeRequest(const std::string& message) override {
        return writeRequest(message, nullptr);
    }

    std::string readResponse(ResponseCallback callback) override;
    void drainPipe(int fd) override;
    void closeAndDeletePipes() override;

    std::atomic<bool> stopIPC_{false};
    void stopIPC() override {
        stopIPC_ = true;
    }

private:
    std::function<std::shared_ptr<ILogHandler>()> logHandler_;
    std::shared_ptr<ILogHandler> log_() { return logHandler_(); };

    std::queue<std::pair<std::string, ResponseCallback>> requestQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    bool isProcessingRequest_;

    // Method to enqueue requests
    bool writeRequestInternal(const std::string& message, ResponseCallback callback);
    void processNextRequest();

    std::atomic<uint64_t> nextRequestId_;

	std::string formatRequest(const std::string& request, uint64_t id);

    void resetResponsePipe();
	
    std::filesystem::path requestPipeFilePath =
        std::filesystem::path(Utils::getHomeDirectory())
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "lim_request"
    ;
    std::filesystem::path responsePipeFilePath =
        std::filesystem::path(Utils::getHomeDirectory())
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "lim_response"
    ;

    std::string requestPipePath = requestPipeFilePath.generic_string();
    std::string responsePipePath = responsePipeFilePath.generic_string();

    std::map<std::string, int> pipes_;

    void removePipeIfExists(const std::string& pipe_name);

    bool createPipe(const std::string& pipe_name);
    bool openPipeForWrite(const std::string& pipe_name, bool non_blocking = false);
    bool openPipeForRead(const std::string& pipe_name, bool non_blocking = false);

};
