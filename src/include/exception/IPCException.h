#pragma once

#include <stdexcept>
#include "Types.h"

class IPCException : public std::runtime_error {
public:
    IPCException(const std::string& message, LogLevel level = LogLevel::LOG_ERROR)
        : std::runtime_error(message), logLevel(level) {}

    [[nodiscard]] auto getLogLevel() const -> LogLevel { return logLevel; }

private:
    LogLevel logLevel;
};
