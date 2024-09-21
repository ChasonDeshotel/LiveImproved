#pragma once

#include <string>
#include <functional>

class ILogHandler;

class IIPC {
public:
    using ResponseCallback = std::function<void(const std::string&)>;

    virtual ~IIPC() = default;

    virtual bool init() = 0;

    virtual bool writeRequest(const std::string& message) = 0;
    virtual bool writeRequest(const std::string& message, ResponseCallback callback) = 0;

    virtual std::string readResponse() = 0;
    virtual void drainPipe(int fd) = 0;
};
