#pragma once

#include "IPCDefinitions.h"
#include "IPCPipe.h"

class IPCRequestPipe : public IPCPipe {
public:
    IPCRequestPipe();
    ~IPCRequestPipe() override;

    IPCRequestPipe(const IPCRequestPipe &) = delete;
    IPCRequestPipe(IPCRequestPipe &&) = delete;
    IPCRequestPipe &operator=(const IPCRequestPipe &) = delete;
    IPCRequestPipe &operator=(IPCRequestPipe &&) = delete;

    auto writeToPipe(ipc::Request) -> bool override;

    auto readResponse(ipc::ResponseCallback callback) -> ipc::Response override;

private:
    ipc::Path   pipePath_;
    ipc::Handle pipeHandle_;
    int         pipeFlags_;
};
