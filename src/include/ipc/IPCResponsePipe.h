#pragma once
#include <filesystem>

#include "Types.h"

#include "IPCPipe.h"

using Path = std::filesystem::path;
namespace fs = std::filesystem;

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
    static constexpr mode_t     DEFAULT_DIRECTORY_PERMISSIONS  {0777};
    static constexpr mode_t     DEFAULT_PIPE_PERMISSIONS       {0666};

    Path       pipePath_;
    PipeHandle pipeHandle_;

};

