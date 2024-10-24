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

private:
};
