#pragma once

#include "ILogHandler.h"
#include <string>
#include <vector>

class MockLogHandler : public ILogHandler {
public:
    void log(const std::string& message, LogLevel level = LogLevel::LOG_INFO) override {
        messages.push_back({logLevelToString(level), message});
    }
    void debug(const std::string& message) override { log(message, LogLevel::LOG_DEBUG); }
    void info(const std::string& message) override { log(message, LogLevel::LOG_INFO); }
    void warn(const std::string& message) override { log(message, LogLevel::LOG_WARN); }
    void error(const std::string& message) override { log(message, LogLevel::LOG_ERROR); }

    void setLogLevel(LogLevel level) override { currentLogLevel = level; }

    const std::vector<std::pair<std::string, std::string>>& getMessages() const { return messages; }
    void clear() { messages.clear(); }

protected:
    std::string logLevelToString(LogLevel level) override {
        switch (level) {
            case LogLevel::LOG_DEBUG: return "DEBUG";
            case LogLevel::LOG_INFO: return "INFO";
            case LogLevel::LOG_WARN: return "WARN";
            case LogLevel::LOG_ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

private:
    std::vector<std::pair<std::string, std::string>> messages;
    LogLevel currentLogLevel = LogLevel::LOG_INFO;
};
