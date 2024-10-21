#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <sys/_types/_useconds_t.h>

#include "IIPC.h"
#include "Types.h"

class IPCQueue : public IIPC {
public:
    using ResponseCallback = std::function<void(const std::string&)>;
    using Request = std::pair<std::string, ResponseCallback>;

    IPCQueue();
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

    void stopIPC() override {
        stopIPC_ = true;
    }

    auto cleanUpPipes() -> void override;

protected:
    auto createReadPipe()  -> bool override;
    auto createWritePipe() -> bool override;
    void createReadPipeLoop();
    void createWritePipeLoop();

    void readyReadPipe();
    void readyWritePipe();

    auto writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool;
    void processNextRequest();
    auto formatRequest(const std::string& request, uint64_t id) -> std::string;

    auto openPipeForWrite(const std::string& pipeName, bool nonBlocking = false) -> bool;
    auto openPipeForRead (const std::string& pipeName, bool nonBlocking = false) -> bool;

    static constexpr int    MAX_READ_RETRIES           {100};
    static constexpr int    MESSAGE_TRUNCATE_CHARS     {100};
    static constexpr int    MAX_PIPE_CREATION_ATTEMPTS {100};
    static constexpr int    MAX_PIPE_SETUP_ATTEMPTS    {100};
    static constexpr size_t BUFFER_SIZE                {8192};

    using ms = std::chrono::milliseconds;
    static constexpr ms     DELAY_BETWEEN_READS        {2000};
    static constexpr ms     PIPE_CREATION_RETRY_DELAY  {500};
    static constexpr ms     PIPE_SETUP_RETRY_DELAY     {500};
    static constexpr ms     LIVE_TICK                  {100};

    static constexpr std::string_view START_MARKER      {"START_"};
    static constexpr std::string_view END_MARKER        {"END_OF_MESSAGE"};
    static constexpr size_t           START_MARKER_SIZE {6};
    static constexpr size_t           REQUEST_ID_SIZE   {8};
    static constexpr size_t           HEADER_SIZE       {START_MARKER_SIZE + REQUEST_ID_SIZE + REQUEST_ID_SIZE};

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
    std::atomic<bool>       readPipeReady_     {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       writePipeReady_    {false};                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    std::queue<Request>     requestQueue_;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex              queueMutex_;                                                 // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable queueCondition_;                                             // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>       isProcessingRequest_{false};                                 // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex              initMutex_   ;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable initCv_      ;                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<uint64_t>   nextRequestId_;                                              // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:

};
