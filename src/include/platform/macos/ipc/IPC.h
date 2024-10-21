#pragma once
#include <filesystem>

#include "Types.h"

using Path = std::filesystem::path;
namespace fs = std::filesystem;

class IPC  {
public:
    IPC()  = default;
    ~IPC() = default;

    IPC(const IPC &) = delete;
    IPC(IPC &&) = delete;
    IPC &operator=(const IPC &) = delete;
    IPC &operator=(IPC &&) = delete;

    static auto createPipe(const Path& pipePath) -> bool;
    static auto cleanUpPipe(const Path& path, PipeHandle& handle) -> void;
    static auto openRequestPipe(const Path& path, PipeHandle& handle) -> bool;
    static auto openResponsePipe(const Path& path, PipeHandle& handle) -> bool;

    static auto drainPipe(int fd, size_t bufferSize) -> void;

private:
    static constexpr mode_t     DEFAULT_DIRECTORY_PERMISSIONS  {0777};
    static constexpr mode_t     DEFAULT_PIPE_PERMISSIONS       {0666};

};
