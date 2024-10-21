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

    auto getHandle() -> ipc::Handle;
    auto setHandleNull() -> void;

    auto string() -> std::string;

    auto drainPipe(int fd, size_t bufferSize) -> void;
    auto stop() -> void {
        stopIPC_ = true;
    }

    auto create() -> bool;
    // static auto resetResponsePipe() -> void;
protected:

    auto setPipePath(const ipc::Path& path) {
        pipePath_ = path;
    }
    virtual auto getPipePath() const -> const ipc::Path& {
        return pipePath_;
    }

    std::atomic<bool> stopIPC_       {false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
private:
    ipc::Path pipePath_;
    ipc::Handle pipeHandle_;

};
