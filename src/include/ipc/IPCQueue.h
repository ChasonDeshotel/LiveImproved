#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#ifndef _WIN32
#include <sys/_types/_useconds_t.h>
#endif
#include <thread>

#include "IIPCQueue.h"
#include "IPCDefinitions.h"
#include "IPCRequestPipe.h"
#include "IPCResponsePipe.h"

class IPCQueue : public IIPCQueue {
public:

    IPCQueue(
            std::function<std::shared_ptr<IPCRequestPipe>()> requestPipe
            , std::function<std::shared_ptr<IPCResponsePipe>()> responsePipe
    );
    ~IPCQueue() override;

    IPCQueue(const IPCQueue &) = delete;
    IPCQueue(IPCQueue &&) = delete;
    auto operator=(const IPCQueue &) -> IPCQueue & = delete;
    auto operator=(IPCQueue &&) -> IPCQueue & = delete;

    auto init() -> ipc::QueueState override;

    [[nodiscard]] auto isInitialized() const -> bool override {
        return currentState_ == ipc::QueueState::Running ? true : false;
    }

    void createRequest(const std::string& message, ipc::ResponseCallback callback) override;
    void createRequest(const std::string& message) override {
        createRequest(message, nullptr);
    }

    auto startProcessing() -> void;
    auto stopProcessing() -> void;

    auto cleanUpPipes() -> void override;

    auto setState(ipc::QueueState newState) -> void {
        auto oldState = currentState_.exchange(newState);
    }

     auto getState() const -> ipc::QueueState {
         return currentState_.load(std::memory_order_acquire);
     }

     auto halt() -> void override;

protected:
    auto readyRequestWrapper() -> void;
    auto readyResponseWrapper() -> void;

    auto pipeObjWrite(ipc::Request) -> bool;
    void processNextRequest();

    // no fucking clue why NOLNTBEGIN (sic) doesn't want to work here
    // protected to allow derived classes direct access
    // consider moving the non-performance-critical ones to private
                                                                                         //
    std::atomic<ipc::QueueState> currentState_ {ipc::QueueState::Initializing}; // NOLINT
                                                                                         //
    std::atomic<bool>       requestPipeReady_  {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       responsePipeReady_ {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    ipc::RequestQueue        requestQueue_;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex               queueMutex_;                                                 // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable  queueCondition_;                                             // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex               initMutex_   ;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable  initCv_      ;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<uint64_t>    nextRequestId_;                                              // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    std::thread processingThread_;
    auto processQueue() -> void;
    auto processRequest(const ipc::Request& request) -> void;

    std::shared_ptr<IPCRequestPipe> requestPipe_;
    std::shared_ptr<IPCResponsePipe> responsePipe_;
};
