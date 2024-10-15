#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <queue>
#include <string>

#include "IIPCCore.h"

class IPCCore : public IIPCCore {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    IPCCore();
    ~IPCCore() override;

    IPCCore(const IPCCore &) = delete;
    IPCCore(IPCCore &&) = delete;
    auto operator=(const IPCCore &) -> IPCCore & = delete;
    auto operator=(IPCCore &&) -> IPCCore & = delete;

    auto init() -> bool override;

    [[nodiscard]] auto isInitialized() const -> bool override {
        return isInitialized_;
    }

    void writeRequest(const std::string& message, ResponseCallback callback) override;
    void writeRequest(const std::string& message) override {
        writeRequest(message, nullptr);
    }

    auto readResponse(ResponseCallback callback) -> std::string override;
    void drainPipe(int fd) override;
    void closeAndDeletePipes() override;

    void stopIPC() override {
        stopIPC_ = true;
    }

private:
    std::atomic<bool> stopIPC_{false};
    static constexpr int MAX_PIPE_CREATION_ATTEMPTS = 100;
    static constexpr std::chrono::milliseconds PIPE_CREATION_RETRY_DELAY{500};
    std::atomic<bool> readPipeCreated_{false};
    std::atomic<bool> writePipeCreated_{false};
    std::condition_variable createPipesCv_;
    std::mutex createPipesMutex_;
    void createPipe();
    void createReadPipe();
    void createWritePipe();

    static constexpr int MAX_PIPE_SETUP_ATTEMPTS = 100;
    static constexpr std::chrono::milliseconds PIPE_SETUP_RETRY_DELAY{500};
    std::atomic<bool> readPipeReady_{false};
    std::atomic<bool> writePipeReady_{false};
    std::atomic<bool> isInitialized_{false};
    std::mutex initMutex_;
    std::condition_variable initCv_;
    void readyReadPipe();
    void readyWritePipe();

    std::queue<std::pair<std::string, ResponseCallback>> requestQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::atomic<bool>isProcessingRequest_{false};

    auto writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool;
    void processNextRequest();
    std::atomic<uint64_t> nextRequestId_;
    auto formatRequest(const std::string& request, uint64_t id) -> std::string;

    void resetResponsePipe();

    std::string requestPipePath_;
    std::string responsePipePath_;

    std::map<std::string, int> pipes_;

    void removePipeIfExists(const std::string& pipeName);

    auto createPipe(const std::string& pipeName) -> bool;
    auto openPipeForWrite(const std::string& pipeName, bool nonBlocking = false) -> bool;
    auto openPipeForRead(const std::string& pipeName, bool nonBlocking = false) -> bool;
};
