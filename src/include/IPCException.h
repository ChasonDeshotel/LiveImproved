#pragma once

#include <stdexcept>
#include "ILogHandler.h"

class IPCException : public std::runtime_error {
public:
    IPCException(const std::string& message, LogLevel level = LogLevel::LOG_ERROR)
        : std::runtime_error(message), logLevel(level) {}

    LogLevel getLogLevel() const { return logLevel; }

private:
    LogLevel logLevel;
};
