#pragma once

#include <string>
#include "Types.h"

class ILogHandler {
public:
    virtual ~ILogHandler() = default;

    ILogHandler(const ILogHandler &) = default;
    ILogHandler(ILogHandler &&) = delete;
    ILogHandler &operator=(const ILogHandler &) = default;
    ILogHandler &operator=(ILogHandler &&) = delete;

    virtual void log(const std::string &message,
                   LogLevel level = LogLevel::LOG_INFO) = 0;
    virtual void debug(const std::string &message) = 0;
    virtual void info(const std::string &message) = 0;
    virtual void warn(const std::string &message) = 0;
    virtual void error(const std::string &message) = 0;

    virtual void setLogLevel(LogLevel level) = 0;

protected:
    ILogHandler() = default;  // Protected default constructor
    virtual auto logLevelToString(LogLevel level) -> std::string = 0;
};
