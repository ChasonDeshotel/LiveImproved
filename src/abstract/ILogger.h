#pragma once
#include <string>

#include "fmt/format.h"

enum class LogLevel { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

enum class LogCategory { CORE, UI, IPC, LIVE, JUCE, MEMORY, PERFORMANCE, RECOVERY };

class LogSink;
class LogHandler;

class ILogger {
public:
    virtual ~ILogger() = default;
    ILogger(ILogger&&) = delete;
    ILogger(const ILogger&) = delete;
    auto operator=(ILogger&&) -> ILogger& = delete;
    auto operator=(const ILogger&) -> ILogger& = delete;

    virtual auto setLogLevel(LogLevel level) -> void = 0;
    virtual auto addSink(const std::shared_ptr<LogSink>& sink) -> void = 0;
    virtual auto setLogPath(const std::string& path) -> void = 0;

    virtual auto toString(LogCategory category) -> std::string = 0;
    virtual auto toString(LogLevel level) -> std::string = 0;

    template <typename... Args>
    void log(fmt::format_string<Args...> fmtstr, Args&&... args) {
        auto message = fmt::format(fmtstr, std::forward<Args>(args)...);
        logImpl(message, LogLevel::LOG_INFO);
    }

    template <typename... Args>
    void trace(fmt::format_string<Args...> fmtstr, Args&&... args) {
        auto message = fmt::format(fmtstr, std::forward<Args>(args)...);
        logImpl(message, LogLevel::LOG_TRACE);
    }

    template <typename... Args>
    void debug(fmt::format_string<Args...> fmtstr, Args&&... args) {
        auto message = fmt::format(fmtstr, std::forward<Args>(args)...);
        logImpl(message, LogLevel::LOG_DEBUG);
    }

    template <typename... Args>
    void info(fmt::format_string<Args...> fmtstr, Args&&... args) {
        auto message = fmt::format(fmtstr, std::forward<Args>(args)...);
        logImpl(message, LogLevel::LOG_INFO);
    }

    template <typename... Args>
    void warn(fmt::format_string<Args...> fmtstr, Args&&... args) {
        auto message = fmt::format(fmtstr, std::forward<Args>(args)...);
        logImpl(message, LogLevel::LOG_WARN);
    }

    template <typename... Args>
    void error(fmt::format_string<Args...> fmtstr, Args&&... args) {
        auto message = fmt::format(fmtstr, std::forward<Args>(args)...);
        logImpl(message, LogLevel::LOG_ERROR);
    }

    void logNoFmt(std::string_view message, LogLevel level = LogLevel::LOG_DEBUG) {
        logImpl(message, level);
    }

protected:
    ILogger() = default;
    virtual void logImpl(std::string_view message, LogLevel level) = 0;
};
