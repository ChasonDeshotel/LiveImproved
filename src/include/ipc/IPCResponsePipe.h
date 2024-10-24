#pragma once

#include "IPCDefinitions.h"
#include "IPCPipe.h"

class PipeUtil;

class IPCResponsePipe : public IPCPipe {
public:
    IPCResponsePipe(std::function<std::shared_ptr<PipeUtil>()> ipcUtil);
    ~IPCResponsePipe() override;

    IPCResponsePipe(const IPCResponsePipe &) = delete;
    IPCResponsePipe(IPCResponsePipe &&) = delete;
    IPCResponsePipe &operator=(const IPCResponsePipe &) = delete;
    IPCResponsePipe &operator=(IPCResponsePipe &&) = delete;

    auto getPipeUtil() -> std::shared_ptr<PipeUtil> {
        return pipeUtil_();
    }

private:
    std::function<std::shared_ptr<PipeUtil>()> pipeUtil_;

    // TODO, delete these and access through pipeutil
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};
