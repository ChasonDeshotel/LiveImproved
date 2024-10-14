#include <iostream>
#include <ctime>
#include <iomanip>
#include <filesystem>  // C++17

#ifndef TEST_BUILD
#include <JuceHeader.h>
#endif

#include "LogHandler.h"

namespace fs = std::filesystem;

LogHandler::LogHandler()
    : ILogHandler()
    , currentLogLevel(LogLevel::LOG_DEBUG) {
    #ifdef _WIN32
        logPath = "C:\\Users\\Billy Maizere\\source\\repos\\LiveImproved\\log.txt";
		//logPath = "NUL";
    #else
        logPath = "/Users/cdeshotel/Scripts/Ableton/LiveImproved/log/debug.txt";
		//logPath = "/dev/null";
    #endif
}

LogHandler::~LogHandler() {
    if (logfile.is_open()) {
        logfile.close();
    }
}

auto LogHandler::getInstance() -> LogHandler& {
    static LogHandler instance;
    return instance;
}

auto LogHandler::setLogPath(const std::string& path) -> void {
    std::lock_guard<std::mutex> lock(logMutex);

    try {
        logPath = fs::path(path).string();
        if (logfile.is_open()) {
            logfile.close();
        }
        logfile.open(logPath, std::ios_base::app);
        if (!logfile.is_open()) {
            std::cerr << "Failed to open log file at: " << logPath << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}

auto LogHandler::setLogLevel(LogLevel level) -> void {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLogLevel = level;
}

auto LogHandler::log(const std::string& message, LogLevel level) -> void {
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

auto LogHandler::debug(const std::string& message) -> void {
    log(message, LogLevel::LOG_DEBUG);
}

auto LogHandler::info(const std::string& message) -> void {
    log(message, LogLevel::LOG_INFO);
    juce::Logger::writeToLog(message);
}

auto LogHandler::warn(const std::string& message) -> void {
    log(message, LogLevel::LOG_WARN);
}

auto LogHandler::error(const std::string& message) -> void {
    log(message, LogLevel::LOG_ERROR);
}

auto LogHandler::logLevelToString(LogLevel level) -> std::string {
    switch (level) {
        case LogLevel::LOG_DEBUG: return "DEBUG";
        case LogLevel::LOG_INFO: return "INFO";
        case LogLevel::LOG_WARN: return "WARN";
        case LogLevel::LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

