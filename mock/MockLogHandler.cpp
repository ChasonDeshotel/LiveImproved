#include "LogHandler.h"

LogHandler& LogHandler::getInstance() {
    static LogHandler instance;
    return instance;
}

void LogHandler::info(const std::string& message) {
    // No-op or store in memory for testing
}

