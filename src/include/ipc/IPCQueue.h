#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <sys/_types/_useconds_t.h>

#include "IIPC.h"
#include "Types.h"
#include "IPCRequestPipe.h"
#include "IPCResponsePipe.h"

class IPCQueue : public IIPC {
public:
    using ResponseCallback = std::function<void(const std::string&)>;
    using Request = std::pair<std::string, ResponseCallback>;

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
        return isInitialized_;
    }

    void writeRequest(const std::string& message, ResponseCallback callback) override;
    void writeRequest(const std::string& message) override {
        writeRequest(message, nullptr);
    }

    auto readResponse(ResponseCallback callback) -> std::string override;

    void stopIPC() override;

    auto cleanUpPipes() -> void override;

protected:
    //auto createReadPipe()  -> bool override;
    //auto createWritePipe() -> bool override;
    //void createReadPipeLoop();
    //void createWritePipeLoop();

    auto readyRequestWrapper() -> void;
    auto readyResponseWrapper() -> void;

    auto writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool;
    void processNextRequest();
    auto formatRequest(const std::string& request, uint64_t id) -> std::string;

    // no fucking clue why NOLNTBEGIN (sic) doesn't want to work here
    // protected to allow derived classes direct access
    // consider moving the non-performance-critical ones to private
    std::atomic<bool> stopIPC_       {false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool> isInitialized_ {false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    std::condition_variable createPipesCv_            ;                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex              createPipesMutex_         ;                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       readPipeCreated_   {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       writePipeCreated_  {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    std::atomic<bool>       requestPipeReady_  {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       responsePipeReady_ {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    std::queue<Request>     requestQueue_;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex              queueMutex_;                                                 // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable queueCondition_;                                             // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       isProcessingRequest_{false};                                 // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex              initMutex_   ;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable initCv_      ;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<uint64_t>   nextRequestId_;                                              // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    std::shared_ptr<IPCRequestPipe> requestPipe_;
    std::shared_ptr<IPCResponsePipe> responsePipe_;
};
