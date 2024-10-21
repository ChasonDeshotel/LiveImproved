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

    virtual auto openPipe() -> bool = 0;
    //virtual auto createWritePipeLoop() -> void = 0;

    auto drainPipe(int fd, size_t bufferSize) -> void;
    auto stop() -> void {
        stopIPC_ = true;
    }

    auto create() -> bool;
    auto ready() -> bool;
    // static auto resetResponsePipe() -> void;
protected:

    auto setPipePath(const ipc::Path& path) {
        pipePath_ = path;
    }
    auto setPipeHandle(const ipc::Handle handle) {
        pipeHandle_ = handle;
    }
    virtual auto getPipePath() const -> const ipc::Path& {
        return pipePath_;
    }
    auto getHandle() -> ipc::Handle {
        return pipeHandle_;
    }
    auto setHandleNull() -> void {
        pipeHandle_ = ipc::NULL_PIPE_HANDLE;
    }

    auto string() -> std::string {
        return pipePath_.string();
    }

    std::atomic<bool> stopIPC_       {false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
private:
    ipc::Path pipePath_;
    ipc::Handle pipeHandle_;

};
