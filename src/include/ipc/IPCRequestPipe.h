#pragma once

#include <functional>

#include "IPCPipe.h"
class PipeUtil;

class IPCRequestPipe : public IPCPipe {
public:
    IPCRequestPipe(std::function<std::shared_ptr<PipeUtil>()> ipcUtil);
    ~IPCRequestPipe() override;

    IPCRequestPipe(const IPCRequestPipe &) = delete;
    IPCRequestPipe(IPCRequestPipe &&) = delete;
    auto operator=(const IPCRequestPipe &) -> IPCRequestPipe & = delete;
    auto operator=(IPCRequestPipe &&) -> IPCRequestPipe & = delete;

private:
};
