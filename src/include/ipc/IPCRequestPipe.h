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

    auto openPipe()  -> bool override;
    
private:
    Path        pipePath_;
    ipc::Handle pipeHandle_;
};
