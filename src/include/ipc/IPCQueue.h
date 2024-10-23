#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <sys/_types/_useconds_t.h>

#include "IIPC.h"
#include "IPCDefinitions.h"
#include "IPCRequestPipe.h"
#include "IPCResponsePipe.h"

class IPCQueue : public IIPC {
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

    auto init() -> bool override;

    [[nodiscard]] auto isInitialized() const -> bool override {
        return currentState_ == ipc::QueueState::Running ? true : false;
    }

    void writeRequest(const std::string& message, ResponseCallback callback) override;
    void writeRequest(const std::string& message) override {
        writeRequest(message, nullptr);
    }

    void stopIPC() override;

    auto cleanUpPipes() -> void override;

     auto setState(ipc::QueueState newState) -> void {
         currentState_.store(newState, std::memory_order_release);
     }

     auto getState() const -> ipc::QueueState {
         return currentState_.load(std::memory_order_acquire);
     }

protected:
    //auto createReadPipe()  -> bool override;
    //auto createWritePipe() -> bool override;
    //void createReadPipeLoop();
    //void createWritePipeLoop();

    auto readyRequestWrapper() -> void;
    auto readyResponseWrapper() -> void;

    auto writeRequestInternal(const std::string& message, ipc::ResponseCallback callback) -> bool;
    void processNextRequest();
    auto formatRequest(const std::string& request, uint64_t id) -> std::string;

    // no fucking clue why NOLNTBEGIN (sic) doesn't want to work here
    // protected to allow derived classes direct access
    // consider moving the non-performance-critical ones to private
    std::atomic<bool> stopIPC_       {false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool> isInitialized_ {false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    std::atomic<ipc::QueueState> currentState_ {ipc::QueueState::Initializing}; // NOLINT
    std::condition_variable createPipesCv_            ;                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex              createPipesMutex_         ;                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       readPipeCreated_   {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       writePipeCreated_  {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    std::atomic<bool>       requestPipeReady_  {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       responsePipeReady_ {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    ipc::RequestQueue        requestQueue_;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex               queueMutex_;                                                 // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable  queueCondition_;                                             // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>        isProcessingRequest_{false};                                 // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex               initMutex_   ;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable  initCv_      ;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<uint64_t>    nextRequestId_;                                              // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    auto createExtendedCallback(const ipc::ResponseCallback& originalCallback) -> ipc::ResponseCallback;
    std::shared_ptr<IPCRequestPipe> requestPipe_;
    std::shared_ptr<IPCResponsePipe> responsePipe_;
};
