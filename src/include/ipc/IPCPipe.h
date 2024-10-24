#pragma once
#include <string>

#include "IPCDefinitions.h"
class PipeUtil;

class IPCPipe {
public:
    IPCPipe(std::shared_ptr<PipeUtil> pipeUtil);
    virtual ~IPCPipe() = default;

    IPCPipe(const IPCPipe &) = delete;
    IPCPipe(IPCPipe &&) = delete;
    IPCPipe &operator=(const IPCPipe &) = delete;
    IPCPipe &operator=(IPCPipe &&) = delete;

    auto getPipeUtil() -> std::shared_ptr<PipeUtil>;

    auto cleanUp()  -> void;

    virtual auto openPipe() -> bool;
    //virtual auto createWritePipeLoop() -> void = 0;

    auto drainPipe() -> void;

    auto stop() -> void {
        stopIPC_ = true;
    }

    auto create() -> bool;
    auto openPipeLoop() -> bool;

    // static auto resetResponsePipe() -> void;

    auto string() -> std::string {
        return pipePath_.string();
    }

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

    auto setHandle(const ipc::Handle handle) {
        pipeHandle_ = handle;
    }
    auto getHandle() -> ipc::Handle {
        return pipeHandle_;
    }
    auto setHandleNull() -> void {
        pipeHandle_ = ipc::NULL_PIPE_HANDLE;
    }

    virtual auto readResponse(ipc::ResponseCallback callback) -> ipc::Response;
    virtual auto writeRequest(ipc::Request) -> bool;
    auto logMessage(const std::string& message) -> void;

protected:
    std::atomic<bool> stopIPC_ {false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    std::shared_ptr<PipeUtil> p_;

    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};
