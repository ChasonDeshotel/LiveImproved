#pragma once

#include <fstream>
#include <mutex>
#include <optional>
#include <string>

#include "LogLevel.h"
#include "ILogHandler.h"

class LogHandler : public ILogHandler {
public:
    LogHandler();
    LogHandler(const LogHandler &) = delete;
    LogHandler(LogHandler &&) = delete;
    auto operator=(const LogHandler &) -> LogHandler & = delete;
    auto operator=(LogHandler &&) -> LogHandler & = delete;
    ~LogHandler() override;

    static auto getInstance() -> LogHandler&;

    auto log(const std::string& message, LogLevel level = LogLevel::LOG_INFO) -> void override;
    auto debug(const std::string& message) -> void override;
    auto info(const std::string& message) -> void override;
    auto warn(const std::string& message) -> void override;
    auto error(const std::string& message) -> void override;

    auto setLogPath(const std::string& path) -> void;
    auto setLogLevel(LogLevel level) -> void override;

private:

    std::ofstream logfile;
    std::optional<std::filesystem::path> logPath;
    LogLevel currentLogLevel;
    std::mutex logMutex;

    auto logLevelToString(LogLevel level) -> std::string override;
};

