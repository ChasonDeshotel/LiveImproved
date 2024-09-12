#ifndef MOCK_LOG_HANDLER_H
#define MOCK_LOG_HANDLER_H

#include <string>

class LogHandler {
public:
    static LogHandler& getInstance();

    void info(const std::string& message);

private:
    LogHandler() = default;
    ~LogHandler() = default;
    LogHandler(const LogHandler&) = delete;
    LogHandler& operator=(const LogHandler&) = delete;
};

#endif
