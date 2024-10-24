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

protected:
    std::atomic<bool> stopIPC_ {false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

private:
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};

