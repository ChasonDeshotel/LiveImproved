#pragma once

#include <string>
#include <functional>

class ILogHandler;

class IIPC {
public:
    virtual ~IIPC() = default;

    virtual bool init() = 0;
    virtual bool writeRequest(const std::string& message) = 0;
    virtual std::string readResponse() = 0;
    virtual bool initReadWithEventLoop(std::function<void(const std::string&)> callback) = 0;
    virtual void drainPipe(int fd) = 0;
};
