#include "MockLogHandler.h"
#include <iostream>
#include <sstream>

MockLogHandler::MockLogHandler()
    : currentLogLevel(LogLevel::LOG_DEBUG) {}

void MockLogHandler::setLogPath(const std::filesystem::path& path) {
    logPath = path;
}

void MockLogHandler::setLogLevel(LogLevel level) {
    currentLogLevel = level;
}

void MockLogHandler::log(const std::string& message, LogLevel level) {
    if (level >= currentLogLevel) {
        std::stringstream ss;
        ss << logLevelToString(level) << ": " << message;
        messages.push_back(ss.str());
    }
}

void MockLogHandler::debug(const std::string& message) {
    log(message, LogLevel::LOG_DEBUG);
}

void MockLogHandler::info(const std::string& message) {
    log(message, LogLevel::LOG_INFO);
}

void MockLogHandler::warn(const std::string& message) {
    log(message, LogLevel::LOG_WARN);
}

void MockLogHandler::error(const std::string& message) {
    log(message, LogLevel::LOG_ERROR);
}

std::string MockLogHandler::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::LOG_DEBUG: return "DEBUG";
        case LogLevel::LOG_INFO: return "INFO";
        case LogLevel::LOG_WARN: return "WARN";
        case LogLevel::LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

const std::vector<std::string>& MockLogHandler::getMessages() const {
    return messages;
}

void MockLogHandler::clear() {
    messages.clear();
}


