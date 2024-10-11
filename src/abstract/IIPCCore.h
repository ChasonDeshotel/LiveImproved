#pragma once

#include <string>
#include <functional>

class IIPCCore {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    virtual ~IIPCCore() = default;

    // Disable copying and moving
    IIPCCore(const IIPCCore&) = delete;
    IIPCCore& operator=(const IIPCCore&) = delete;
    IIPCCore(IIPCCore&&) = delete;
    IIPCCore& operator=(IIPCCore&&) = delete;

    virtual bool init() = 0;
    virtual bool isInitialized() const = 0;

    virtual void writeRequest(const std::string& message) = 0;
    virtual void writeRequest(const std::string& message, ResponseCallback callback) = 0;

    virtual auto readResponse(ResponseCallback callback) -> std::string = 0;
    virtual void drainPipe(int fd) = 0;
    virtual void closeAndDeletePipes() = 0;

    virtual void stopIPC() = 0;

protected:
    IIPCCore() = default;  // Protected default constructor
};
