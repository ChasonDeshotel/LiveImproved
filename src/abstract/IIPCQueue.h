#pragma once

#include <string>
#include <filesystem>

#include "IPCDefinitions.h"

using Path = std::filesystem::path;

class IIPCQueue {
public:
    virtual ~IIPCQueue() = default;

    IIPCQueue(const IIPCQueue&) = delete;
    auto operator=(const IIPCQueue&) -> IIPCQueue& = delete;
    IIPCQueue(IIPCQueue&&) = delete;
    auto operator=(IIPCQueue&&) -> IIPCQueue& = delete;

    virtual auto init() -> ipc::QueueState = 0;
    [[nodiscard]] virtual auto isInitialized() const -> bool = 0;

    virtual auto createRequest(const std::string& message) -> void = 0;
    virtual auto createRequest(const std::string& message, ipc::ResponseCallback callback) -> void = 0;

    virtual auto halt() -> void = 0;

    virtual auto cleanUpPipes() -> void = 0;

protected:
    IIPCQueue() = default;
};
