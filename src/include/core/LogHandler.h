#pragma once

#include <fstream>
#include <mutex>
#include <optional>
#include <string>

#include "Types.h"
#include "ILogHandler.h"

class LogHandler : public ILogHandler {
public:
    LogHandler();
    LogHandler(const LogHandler &) = delete;
    LogHandler(LogHandler &&) = delete;
    LogHandler &operator=(const LogHandler &) = delete;
    LogHandler &operator=(LogHandler &&) = delete;
    ~LogHandler() override;

    static LogHandler& getInstance();

    void log(const std::string& message, LogLevel level = LogLevel::LOG_INFO) override;
    void debug(const std::string& message) override;
    void info(const std::string& message) override;
    void warn(const std::string& message) override;
    void error(const std::string& message) override;

    void setLogPath(const std::string& path);
    void setLogLevel(LogLevel level) override;

private:

    std::ofstream logfile;
    std::optional<std::filesystem::path> logPath;
    LogLevel currentLogLevel;
    std::mutex logMutex;

    std::string logLevelToString(LogLevel level) override;
};

