#pragma once
#include <filesystem>

#include "IPCBase.h"

using Path = std::filesystem::path;
namespace fs = std::filesystem;

class IPC : public IPCBase {
public:
    IPC();
    ~IPC() override;

    IPC(const IPC &) = delete;
    IPC(IPC &&) = delete;
    IPC &operator=(const IPC &) = delete;
    IPC &operator=(IPC &&) = delete;

    auto createReadPipe() -> bool override;
    auto createWritePipe() -> bool override;

    auto cleanUpPipe(const Path& path, PipeHandle& handle) -> void override;

private:
    auto createPipe(const Path& pipePath) -> bool;
};
