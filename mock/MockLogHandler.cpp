#include "MockLogHandler.h"

LogHandler& LogHandler::getInstance() {
    static LogHandler instance;
    return instance;
}

void LogHandler::debug(const std::string& message) {
    // No-op or store in memory for testing
}

void LogHandler::info(const std::string& message) {
    // No-op or store in memory for testing
}

void LogHandler::warn(const std::string& message) {
    // No-op or store in memory for testing
}

void LogHandler::error(const std::string& message) {
    // No-op or store in memory for testing
}
