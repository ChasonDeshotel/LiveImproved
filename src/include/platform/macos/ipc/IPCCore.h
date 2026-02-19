#pragma once
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <string>
#include <thread>
#include "IIPCCore.h"

class IPCCore : public IIPCCore {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    IPCCore() = default;
    ~IPCCore() override;
    IPCCore(const IPCCore&) = delete;
    IPCCore(IPCCore&&) = delete;
    auto operator=(const IPCCore&) -> IPCCore& = delete;
    auto operator=(IPCCore&&) -> IPCCore& = delete;

    void init() override;
    void readLoop();
    void processBuffer(std::string& buffer);
    auto isInitialized() const -> bool override { return isInitialized_; }
    void writeRequest(const std::string& message, ResponseCallback callback) override;
    void writeRequest(const std::string& message) override { writeRequest(message, nullptr); }
    auto readResponse(uint64_t id, ResponseCallback callback) -> std::string override;
    void stopIPC() override { stopIPC_ = true; }
    void destroy() override;
    bool isInitialized() { return isInitialized_; }

    // keeping for interface compat, noop now
    void drainPipe(int fd) override {}
    void closeAndDeletePipes() override {}

private:
    static constexpr int PORT                   = 47474;
    static constexpr size_t BUFFER_SIZE         = 8192;
    static constexpr int MESSAGE_TRUNCATE_CHARS = 100;

    std::mutex responseMutex_;
    std::condition_variable responseCv_;
    //std::queue<std::string> responseQueue_;
    std::map<uint64_t, std::string> responseQueue_;

    std::atomic<bool> stopIPC_{false};
    std::atomic<bool> isInitialized_{false};
    std::atomic<uint64_t> nextRequestId_{420};
    std::atomic<bool> isReady_{false};

    int serverFd_{-1};
    int clientFd_{-1};

    std::thread initThread_;
    std::thread acceptThread_;
    std::thread readThread_;
    std::thread writeThread_;

    std::mutex writeMutex_;
    std::queue<std::pair<std::string, ResponseCallback>> requestQueue_;
    std::mutex queueMutex_;
    std::atomic<bool> isProcessingRequest_{false};

    auto formatRequest(const std::string& request, uint64_t id) -> std::string;
    auto writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool;
    void processNextRequest();
};
