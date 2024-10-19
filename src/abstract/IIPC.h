#pragma once

#include <string>
#include <functional>

class IIPC {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    virtual ~IIPC() = default;

    IIPC(const IIPC&) = delete;
    auto operator=(const IIPC&) -> IIPC& = delete;
    IIPC(IIPC&&) = delete;
    auto operator=(IIPC&&) -> IIPC& = delete;

    virtual auto init() -> bool = 0;
    [[nodiscard]] virtual auto isInitialized() const -> bool = 0;

    virtual auto writeRequest(const std::string& message) -> void = 0;
    virtual auto writeRequest(const std::string& message, ResponseCallback callback) -> void = 0;

    virtual auto readResponse(ResponseCallback callback) -> std::string = 0;
    virtual auto drainPipe(int fd) -> void = 0;
    virtual auto closeAndDeletePipes() -> void = 0;

    virtual auto stopIPC() -> void = 0;

protected:
    IIPC() = default;
};
