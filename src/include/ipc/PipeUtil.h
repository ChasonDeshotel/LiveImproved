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

    auto writeToPipe(ipc::Request request) -> size_t;
    
    auto setHandle(ipc::Handle handle) -> void;
    auto setPath(const ipc::Path& path) -> void;
    auto setFlags(int flags) -> void;

    auto getHandle() const -> ipc::Handle;
    auto getPath() const -> const ipc::Path&;
    auto getFlags() const -> int;

protected:
    std::atomic<bool> stopIPC_ {false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};

