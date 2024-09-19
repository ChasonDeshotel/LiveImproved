#pragma once

#include <string>

enum class LogLevel {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

class ILogHandler {
public:
    virtual ~ILogHandler() = default;

    virtual void log(const std::string& message, LogLevel level = LogLevel::LOG_INFO) = 0;
    virtual void debug(const std::string& message) = 0;
    virtual void info(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;

    virtual void setLogLevel(LogLevel level) = 0;

protected:
    virtual std::string logLevelToString(LogLevel level) = 0;
};
