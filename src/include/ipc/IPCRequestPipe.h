#pragma once

#include <functional>

#include "IPCDefinitions.h"
#include "IPCPipe.h"
class PipeUtil;

class IPCRequestPipe : public IPCPipe {
public:
    IPCRequestPipe(std::function<std::shared_ptr<PipeUtil>()> ipcUtil);
    ~IPCRequestPipe() override;

    IPCRequestPipe(const IPCRequestPipe &) = delete;
    IPCRequestPipe(IPCRequestPipe &&) = delete;
    IPCRequestPipe &operator=(const IPCRequestPipe &) = delete;
    IPCRequestPipe &operator=(IPCRequestPipe &&) = delete;

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
