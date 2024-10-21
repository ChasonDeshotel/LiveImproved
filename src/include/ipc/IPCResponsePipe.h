#pragma once

#include "IPCDefinitions.h"
#include "IPCPipe.h"

class IPCResponsePipe : public IPCPipe {
public:
    IPCResponsePipe();
    ~IPCResponsePipe() override;

    IPCResponsePipe(const IPCResponsePipe &) = delete;
    IPCResponsePipe(IPCResponsePipe &&) = delete;
    IPCResponsePipe &operator=(const IPCResponsePipe &) = delete;
    IPCResponsePipe &operator=(IPCResponsePipe &&) = delete;

    auto openPipe() -> bool override;

private:
    Path        pipePath_;
    ipc::Handle pipeHandle_;
};
