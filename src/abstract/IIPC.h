#pragma once

#include <string>
#include <filesystem>

#include "IPCDefinitions.h"

using Path = std::filesystem::path;

class IIPC {
public:

    virtual ~IIPC() = default;

    IIPC(const IIPC&) = delete;
    auto operator=(const IIPC&) -> IIPC& = delete;
    IIPC(IIPC&&) = delete;
    auto operator=(IIPC&&) -> IIPC& = delete;

    virtual auto init() -> bool = 0;
    [[nodiscard]] virtual auto isInitialized() const -> bool = 0;

    virtual auto createRequest(const std::string& message) -> void = 0;
    virtual auto createRequest(const std::string& message, ipc::ResponseCallback callback) -> void = 0;

    virtual auto stopIPC() -> void = 0;

    virtual auto cleanUpPipes() -> void = 0;

protected:
    IIPC() = default;
};
