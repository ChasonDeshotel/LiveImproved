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
    ~LogHandler();

    static LogHandler& getInstance();

    void log(const std::string& message, LogLevel level = LogLevel::LOG_INFO);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    void setLogPath(const std::string& path);
    void setLogLevel(LogLevel level);

private:

    std::ofstream logfile;
    std::optional<std::filesystem::path> logPath;
    LogLevel currentLogLevel;
    std::mutex logMutex;

    std::string logLevelToString(LogLevel level);
};

