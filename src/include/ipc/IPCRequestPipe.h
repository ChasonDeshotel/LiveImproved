#pragma once
#include <filesystem>

#include "Types.h"

#include "IPCPipe.h"

using Path = std::filesystem::path;
namespace fs = std::filesystem;

class IPCRequestPipe : public IPCPipe {
public:
    IPCRequestPipe();
    ~IPCRequestPipe() override;

    IPCRequestPipe(const IPCRequestPipe &) = delete;
    IPCRequestPipe(IPCRequestPipe &&) = delete;
    IPCRequestPipe &operator=(const IPCRequestPipe &) = delete;
    IPCRequestPipe &operator=(IPCRequestPipe &&) = delete;

    auto openPipe()  -> bool override;
    //auto createWritePipeLoop() -> void override;

private:

    Path       pipePath_;
    PipeHandle pipeHandle_;

};
