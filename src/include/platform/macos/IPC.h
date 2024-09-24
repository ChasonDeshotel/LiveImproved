#pragma once

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

struct RequestInfo {
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point writeTime;
    std::chrono::system_clock::time_point responseTime;
    std::string status;
};

class IPC : public IIPC {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    IPC(
        std::function<std::shared_ptr<ILogHandler>()> logHandler
       );
    ~IPC() override;

    bool init() override;
    
    bool writeRequest(const std::string& message, ResponseCallback callback) override;
    bool writeRequest(const std::string& message) override {
        return writeRequest(message, nullptr);
    }

    std::string readResponse() override;
    void drainPipe(int fd) override;

private:
    std::function<std::shared_ptr<ILogHandler>()> logHandler_;
    std::shared_ptr<ILogHandler> log_() { return logHandler_(); };

	std::map<uint64_t, RequestInfo> requestTracker_;
	void trackRequest(uint64_t id, const std::string& status);

    std::queue<std::string> requestQueue_;
    std::map<uint64_t, ResponseCallback> pendingCallbacks_;
    std::atomic<uint64_t> nextRequestId_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread requestWriterThread_;
    std::thread responseReaderThread_;
    std::atomic<bool> shouldStop_{false};

	std::string formatRequest(const std::string& request, uint64_t id);

	void processResponse(const std::string& response);
	void requestWriter();
	void responseReader();
	
    std::string readResponseInternal(int fd);
    bool writeRequestInternal(const std::string& message);

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

    bool writeToPipe(const std::string& pipe_name, const std::string& message);
    std::string readFromPipe(const std::string& pipe_name);
};
