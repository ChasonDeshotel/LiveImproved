#pragma once

#include <string>
#include <functional>
#include <filesystem>

#ifdef _WIN32
struct HANDLE__;
using PipeHandle = struct HANDLE__*;
static constexpr PipeHandle UNINITIALIZED_HANDLE = nullptr;
#else
using PipeHandle = int;
static constexpr PipeHandle NULL_PIPE_HANDLE = -69420;
static constexpr PipeHandle INVALID_PIPE_HANDLE = -1;
#endif

using Path = std::filesystem::path;

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
    virtual auto cleanUpPipes() -> void = 0;

    IIPC() = default;
};
