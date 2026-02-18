#pragma once
#include <string>

class LogSink {
public:
    virtual ~LogSink() = default;
    LogSink(LogSink&&) = delete;
    LogSink(const LogSink&) = delete;
    auto operator=(LogSink&&) -> LogSink& = delete;
    auto operator=(const LogSink&) -> LogSink& = delete;

    virtual void write(std::string_view formattedMessage) = 0;
    virtual auto isAvailable() const -> bool { return true; }
    virtual void flush() {}

protected:
    LogSink() = default;
};