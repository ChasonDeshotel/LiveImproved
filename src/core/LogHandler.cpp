#include <iostream>
#include <ctime>
#include <filesystem>
#include <optional>


#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

#ifndef TEST_BUILD
#include <JuceHeader.h>
#endif

#include "LogHandler.h"
#include "PathManager.h"

namespace fs = std::filesystem;

LogHandler::LogHandler()
    : ILogHandler()
    , currentLogLevel(LogLevel::LOG_DEBUG) {
    logPath = PathManager().log();
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

auto LogHandler::setLogPath(const std::filesystem::path& path) -> void {
    std::lock_guard<std::mutex> lock(logMutex);

    try {
        if (logfile.is_open()) {
            logfile.close();
        }
        
        // Create directories if they don't exist
        fs::create_directories(path.parent_path());

        logfile.open(path.string(), std::ios_base::app);
        if (!logfile.is_open()) {
            std::cerr << "Failed to open log file at: " << path.string() << std::endl;
        } else {
            logPath = path;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        logPath = std::nullopt;
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

    #ifdef TEST_BUILD
    std::cout << logLevelToString(level) << ": " << message << std::endl;
    #else
    juce::Logger::writeToLog(message);
    #endif

    std::cerr << message << std::endl;
//
//    if (!logPath.has_value()) {
//        std::cerr << logLevelToString(level) << ": " << message << std::endl;
//        return;
//    }

    //if (!logfile.is_open()) {
    //    logfile.open(logPath->string(), std::ios_base::app);
    //}

    if (logfile.is_open()) {
        auto now = std::time(nullptr);
        #ifdef _WIN32
        std::tm timeInfo;
        localtime_s(&timeInfo, &now);
        auto localTime = &timeInfo;
        #else
        auto localTime = std::localtime(&now);
        #endif

        logfile << "[" << std::put_time(localTime, "%Y-%m-%d %H:%M:%S") << "] "
                << logLevelToString(level) << ": " << message << std::endl;
    } else {
        std::cerr << "Unable to open log file: " << logPath->string() << std::endl;
    }
}

auto LogHandler::debug(const std::string& message) -> void {
    log(message, LogLevel::LOG_DEBUG);
}

auto LogHandler::info(const std::string& message) -> void {
    log(message, LogLevel::LOG_INFO);
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
        case LogLevel::LOG_INFO:  return "INFO";
        case LogLevel::LOG_WARN:  return "WARN";
        case LogLevel::LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

