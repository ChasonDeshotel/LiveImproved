#pragma once
#include <string>

#include "IPCDefinitions.h"

class IPCPipe {
public:
    IPCPipe();
    virtual ~IPCPipe() = default;

    IPCPipe(const IPCPipe &) = delete;
    IPCPipe(IPCPipe &&) = delete;
    IPCPipe &operator=(const IPCPipe &) = delete;
    IPCPipe &operator=(IPCPipe &&) = delete;

    auto cleanUp()  -> void;

    virtual auto openPipe() -> bool;
    //virtual auto createWritePipeLoop() -> void = 0;

    auto drainPipe(int fd, size_t bufferSize) -> void;
    auto stop() -> void {
        stopIPC_ = true;
    }

    auto create() -> bool;
    auto openPipeLoop() -> bool;

    // static auto resetResponsePipe() -> void;

    auto string() -> std::string {
        return pipePath_.string();
    }

protected:
    std::atomic<bool> stopIPC_ {false};                           // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

    auto setPipePath(const ipc::Path& path) -> void {
        pipePath_ = path;
    }
    virtual auto getPipePath() const -> const ipc::Path& {
        return pipePath_;
    }

    auto setPipeFlags(const int flags) -> void {
        pipeFlags_ = flags;
    }
    auto getPipeFlags() -> int {
        return pipeFlags_;
    }

    auto setPipeHandle(const ipc::Handle handle) {
        pipeHandle_ = handle;
    }
    auto getHandle() -> ipc::Handle {
        return pipeHandle_;
    }

    auto setHandleNull() -> void {
        pipeHandle_ = ipc::NULL_PIPE_HANDLE;
    }
                                                                                         //
private:
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};
