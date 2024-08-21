#include "LogHandler.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <filesystem>  // C++17

namespace fs = std::filesystem;

LogHandler::LogHandler() : currentLogLevel(LogLevel::INFO) {
    logPath = "/Users/cdeshotel/Scripts/Ableton/InterceptKeys/log.txt";
}

LogHandler::~LogHandler() {
    if (logfile.is_open()) {
        logfile.close();
    }
}

LogHandler& LogHandler::getInstance() {
    static LogHandler instance;
    return instance;

}

void LogHandler::setLogPath(const std::string& path) {
    std::lock_guard<std::mutex> lock(logMutex);
    logPath = fs::path(path).string();  // Ensure the path is handled correctly
    if (logfile.is_open()) {
        logfile.close();
    }
    logfile.open(logPath, std::ios_base::app);
}

void LogHandler::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLogLevel = level;
}

void LogHandler::log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (level < currentLogLevel) {
        return;
    }

    if (!logfile.is_open()) {
        logfile.open(logPath, std::ios_base::app);
    }

    if (logfile.is_open()) {
        auto now = std::time(nullptr);
        auto localTime = std::localtime(&now);

        logfile << "[" << std::put_time(localTime, "%Y-%m-%d %H:%M:%S") << "] "
                << logLevelToString(level) << ": " << message << std::endl;
    } else {
        std::cerr << "Unable to open log file: " << logPath << std::endl;
    }
}

void LogHandler::debug(const std::string& message) {
    log(message, LogLevel::DEBUG);
}

void LogHandler::info(const std::string& message) {
    log(message, LogLevel::INFO);
}

void LogHandler::warn(const std::string& message) {
    log(message, LogLevel::WARN);
}

void LogHandler::error(const std::string& message) {
    log(message, LogLevel::ERROR);
}

std::string LogHandler::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

