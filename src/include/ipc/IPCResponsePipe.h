#pragma once

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
};
