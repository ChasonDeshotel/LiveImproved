#ifndef MOCK_LOG_HANDLER_H
#define MOCK_LOG_HANDLER_H

#include <string>

class LogHandler {
public:
    static LogHandler& getInstance();

    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    LogHandler() = default;
    ~LogHandler() = default;
    LogHandler(const LogHandler&) = delete;
    LogHandler& operator=(const LogHandler&) = delete;
};

#endif
