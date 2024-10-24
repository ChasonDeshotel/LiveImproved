#pragma once

#include "IPCDefinitions.h"

class PipeUtil {
public:
    PipeUtil();
    ~PipeUtil() = default;

    PipeUtil(const PipeUtil &) = delete;
    PipeUtil(PipeUtil &&) = delete;
    PipeUtil &operator=(const PipeUtil &) = delete;
    PipeUtil &operator=(PipeUtil &&) = delete;

    auto createPipe() -> bool;
    auto openPipe() -> bool;
    auto closePipe() -> bool;
    auto drainPipe() -> void;
    auto deletePipe() -> void;

    auto ensurePipeOpen() -> bool;

    auto writeToPipe(ipc::Request request) -> size_t;
    auto readFromPipe(void* buffer, size_t count) const -> ssize_t;
    
    auto setHandle(ipc::Handle handle) -> void {
        pipeHandle_ = handle;
    }

    auto setPath(const ipc::Path& path) -> void {
        logger->error("setting path: " + path.string());
        pipePath_ = path;
    }

    auto setFlags(int flags) -> void {
        logger->error("setting flags");
        pipeFlags_ = flags;
    }

    auto getHandle() const -> ipc::Handle {
        return pipeHandle_;
    }

    auto getPath() const -> const ipc::Path& {
        return pipePath_;
    }

    auto getFlags() const -> int {
        return pipeFlags_;
    }

protected:
    std::atomic<bool> stopIPC_ {false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};

