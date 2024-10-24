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

    auto create() -> bool;
    void setHandle(ipc::Handle handle);
    void setPath(const ipc::Path& path);
    void setFlags(int flags);

protected:
    std::atomic<bool> stopIPC_ {false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};

