#pragma once

#include <string>

#include "LogLevel.h"

class ILogHandler {
public:
    virtual ~ILogHandler() = default;

    ILogHandler(const ILogHandler &) = default;
    ILogHandler(ILogHandler &&) = delete;
    auto operator=(const ILogHandler &) -> ILogHandler & = default;
    auto operator=(ILogHandler &&) -> ILogHandler & = delete;

    virtual auto log(const std::string &message,
                   LogLevel level = LogLevel::LOG_INFO) -> void = 0;
    virtual auto debug(const std::string &message) -> void = 0;
    virtual auto info(const std::string &message) -> void = 0;
    virtual auto warn(const std::string &message) -> void = 0;
    virtual auto error(const std::string &message) -> void = 0;

    virtual auto setLogLevel(LogLevel level) -> void = 0;

protected:
    ILogHandler() = default;  // Protected default constructor
    virtual auto logLevelToString(LogLevel level) -> std::string = 0;
};
