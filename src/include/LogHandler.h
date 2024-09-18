#ifndef LOG_HANDLER_H
#define LOG_HANDLER_H

#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel {
    LOG_TRACE     // For detailed tracing information
    , LOG_DEBUG   // Renamed from DEBUG to avoid macro conflict
    , LOG_INFO    // For informational messages
    , LOG_WARN    // For warnings
    , LOG_ERROR   // For error messages
    , LOG_FATAL   // For critical issues
};

class LogHandler {
public:
    static LogHandler& getInstance();

    void log(const std::string& message, LogLevel level = LogLevel::LOG_INFO);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    void setLogPath(const std::string& path);
    void setLogLevel(LogLevel level);

private:
    LogHandler();
    ~LogHandler();
    LogHandler(const LogHandler&) = delete;
    LogHandler& operator=(const LogHandler&) = delete;

    std::ofstream logfile;
    std::string logPath;
    LogLevel currentLogLevel;
    std::mutex logMutex;

    std::string logLevelToString(LogLevel level);


};

#endif
