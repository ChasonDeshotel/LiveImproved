#pragma once

#include <string>
#include <functional>

class IIPCCore {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    virtual ~IIPCCore() = default;

    IIPCCore(const IIPCCore&) = delete;
    auto operator=(const IIPCCore&) -> IIPCCore& = delete;
    IIPCCore(IIPCCore&&) = delete;
    auto operator=(IIPCCore&&) -> IIPCCore& = delete;

    virtual void init() = 0;
    [[nodiscard]] virtual auto isInitialized() const -> bool = 0;

    virtual auto writeRequest(const std::string& message) -> void = 0;
    virtual auto writeRequest(const std::string& message, ResponseCallback callback) -> void = 0;

    virtual auto readResponse(uint64_t id, ResponseCallback callback) -> std::string = 0;
    virtual auto drainPipe(int fd) -> void = 0;
    virtual auto closeAndDeletePipes() -> void = 0;

    virtual auto stopIPC() -> void = 0;
    virtual auto destroy() -> void = 0;

protected:
    IIPCCore() = default;
};
