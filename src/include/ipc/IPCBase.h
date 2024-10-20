#pragma once

#include <atomic>
#include <filesystem>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <sys/_types/_useconds_t.h>

#include "IIPC.h"


class IPCBase : public IIPC {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    IPCBase();
    ~IPCBase() override;

    IPCBase(const IPCBase &) = delete;
    IPCBase(IPCBase &&) = delete;
    auto operator=(const IPCBase &) -> IPCBase & = delete;
    auto operator=(IPCBase &&) -> IPCBase & = delete;

    auto init() -> bool override;

    [[nodiscard]] auto isInitialized() const -> bool override {
        return _isInitialized_;
    }

    void writeRequest(const std::string& message, ResponseCallback callback) override;
    void writeRequest(const std::string& message) override {
        writeRequest(message, nullptr);
    }

    auto readResponse(ResponseCallback callback) -> std::string override;
    void drainPipe(int fd) override;
    void closeAndDeletePipes() override;

    void stopIPC() override {
        _stopIPC_ = true;
    }

protected:
    void createReadPipeLoop();
    void createWritePipeLoop();
    virtual bool createReadPipe();
    virtual bool createWritePipe();

    void readyReadPipe();
    void readyWritePipe();

    auto cleanUpPipe(const Path& path, PipeHandle& handle) -> void;
    auto cleanUpPipes() -> void override;

    auto writeRequestInternal(const std::string& message, ResponseCallback callback) -> bool;
    void processNextRequest();
    auto formatRequest(const std::string& request, uint64_t id) -> std::string;

    void resetResponsePipe();

    void removePipeIfExists(const std::string& pipeName);

    auto openPipeForWrite(const std::string& pipeName, bool nonBlocking = false) -> bool;
    auto openPipeForRead(const std::string& pipeName, bool nonBlocking = false) -> bool;
    static constexpr mode_t DEFAULT_DIRECTORY_PERMISSIONS = 0777;
    static constexpr mode_t DEFAULT_PIPE_PERMISSIONS      = 0666;

    static constexpr useconds_t DELAY_BETWEEN_READS {20000};
    static constexpr int MAX_READ_RETRIES           {100};
    static constexpr int MESSAGE_TRUNCATE_CHARS     {100};
    static constexpr int MAX_PIPE_CREATION_ATTEMPTS {100};
    static constexpr int MAX_PIPE_SETUP_ATTEMPTS    {100};
    static constexpr size_t BUFFER_SIZE             {8192};

    static constexpr std::chrono::milliseconds PIPE_CREATION_RETRY_DELAY {500};
    static constexpr std::chrono::milliseconds PIPE_SETUP_RETRY_DELAY    {500};

    static constexpr std::string_view START_MARKER {"START_"};
    static constexpr std::string_view END_MARKER   {"END_OF_MESSAGE"};

    // no fucking clue why NOLNTBEGIN (sic) doesn't want to work here
    // protected to allow derived classes direct access
    // consider moving the non-performance-critical ones to private
    std::atomic<bool> _stopIPC_{false};                                                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool> _isInitialized_{false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    std::map<std::string, int> pipes_;                                                   // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    Path _requestPipePath_;                                                              // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    Path _responsePipePath_;                                                             // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    PipeHandle _requestPipeHandle_;                                                      // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    PipeHandle _responsePipeHandle_;                                                     // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool> _readPipeCreated_{false};                                          // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool> _writePipeCreated_{false};                                         // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable _createPipesCv_;                                             // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex _createPipesMutex_;                                                       // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool> _readPipeReady_{false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool> _writePipeReady_{false};                                           // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    std::queue<std::pair<std::string, ResponseCallback>> requestQueue_;                  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex queueMutex_;                                                              // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable queueCondition_;                                             // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<bool>isProcessingRequest_{false};                                        // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::mutex initMutex_;                                                               // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::condition_variable initCv_;                                                     // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    std::atomic<uint64_t> nextRequestId_;                                                // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:

};
