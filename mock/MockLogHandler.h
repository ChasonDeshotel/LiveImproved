#pragma once

#include "ILogHandler.h"
#include <string>
#include <vector>
#include <filesystem>

class MockLogHandler : public ILogHandler {
public:
    MockLogHandler();

    void log(const std::string& message, LogLevel level = LogLevel::LOG_INFO) override;
    void debug(const std::string& message) override;
    void info(const std::string& message) override;
    void warn(const std::string& message) override;
    void error(const std::string& message) override;

    void setLogLevel(LogLevel level) override;
    void setLogPath(const std::filesystem::path& path) override;

    const std::vector<std::string>& getMessages() const;
    void clear();

protected:
    std::string logLevelToString(LogLevel level) override;

private:
    std::vector<std::string> messages;
    LogLevel currentLogLevel;
    std::filesystem::path logPath;
};
