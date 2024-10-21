#pragma once
#include <filesystem>
#include <string>

#include "Types.h"

using Path = std::filesystem::path;
namespace fs = std::filesystem;

class IPCPipe {
public:
    IPCPipe();
    virtual ~IPCPipe() = default;

    IPCPipe(const IPCPipe &) = delete;
    IPCPipe(IPCPipe &&) = delete;
    IPCPipe &operator=(const IPCPipe &) = delete;
    IPCPipe &operator=(IPCPipe &&) = delete;

    auto create()   -> bool;
    auto cleanUp()  -> void;

    virtual auto openPipe() -> bool = 0;
    virtual auto createWritePipeLoop() -> void = 0;

    auto getHandle() -> PipeHandle;
    auto setHandleNull() -> void;

    auto string() -> std::string;

    auto drainPipe(int fd, size_t bufferSize) -> void;

    // static auto resetResponsePipe() -> void;
protected:
    // TODO fix
    std::atomic<bool> stopIPC_       {false};                                            // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
                                                                                         //
    static constexpr int    MAX_PIPE_CREATION_ATTEMPTS {100};
    static constexpr int    MAX_PIPE_SETUP_ATTEMPTS    {100};

    static constexpr mode_t     DEFAULT_DIRECTORY_PERMISSIONS  {0777};
    static constexpr mode_t     DEFAULT_PIPE_PERMISSIONS       {0666};

private:
    Path       pipePath_;
    PipeHandle pipeHandle_;

};
