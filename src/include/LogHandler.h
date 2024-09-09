#ifndef LOG_HANDLER_H
#define LOG_HANDLER_H

#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class LogHandler {
public:
    static LogHandler& getInstance();

    void log(const std::string& message, LogLevel level = LogLevel::INFO);
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
