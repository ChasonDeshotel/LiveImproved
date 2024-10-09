#pragma once

#include <string>
#include <functional>

class ILogHandler;

class IIPC {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    virtual ~IIPC() = default;

    // Disable copying and moving
    IIPC(const IIPC&) = delete;
    IIPC& operator=(const IIPC&) = delete;
    IIPC(IIPC&&) = delete;
    IIPC& operator=(IIPC&&) = delete;

    virtual bool init() = 0;

    virtual void writeRequest(const std::string& message) = 0;
    virtual void writeRequest(const std::string& message, ResponseCallback callback) = 0;

    virtual auto readResponse(ResponseCallback callback) -> std::string = 0;
    virtual void drainPipe(int fd) = 0;
    virtual void closeAndDeletePipes() = 0;

    virtual void stopIPC() = 0;

protected:
    IIPC() = default;  // Protected default constructor
};
