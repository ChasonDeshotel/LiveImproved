#include <iostream>
#include <ctime>
#include <filesystem>
#include <optional>
#include <sstream>

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
    initializeLogFile();
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

auto LogHandler::initializeLogFile() -> void {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!logPath.has_value()) {
        std::cerr << "Log path not set." << std::endl;
        return;
    }

    try {
        // Create directories if they don't exist
        fs::create_directories(logPath->parent_path());

        logfile.open(logPath->string(), std::ios_base::app);
        if (!logfile.is_open()) {
            std::cerr << "Failed to open log file at: " << logPath->string() << std::endl;
            logPath = std::nullopt;
        } else {
            std::cerr << "Successfully opened log file at: " << logPath->string() << std::endl;
            logfile << "Log file opened at: " << getCurrentTimestamp() << std::endl;
            logfile.flush();
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        logPath = std::nullopt;
    }
}

auto LogHandler::setLogPath(const std::filesystem::path& path) -> void {
    std::lock_guard<std::mutex> lock(logMutex);

    if (logfile.is_open()) {
        logfile.close();
    }

    logPath = path;
    initializeLogFile();
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

    std::string formattedMessage = formatLogMessage(message, level);

    #ifdef TEST_BUILD
    std::cout << formattedMessage << std::endl;
    #else
    juce::Logger::writeToLog(formattedMessage);
    #endif

    if (!logPath.has_value()) {
        std::cerr << "Log path not set. " << formattedMessage << std::endl;
        return;
    }

    if (!logfile.is_open()) {
        initializeLogFile();
        if (!logfile.is_open()) {
            std::cerr << "Failed to open log file at: " << logPath->string() << std::endl;
            return;
        }
    }

    logfile << formattedMessage << std::endl;
    
    // Flush the stream to ensure the message is written immediately
    logfile.flush();

    if (logfile.fail()) {
        std::cerr << "Failed to write to log file: " << logPath->string() << std::endl;
        // Try to get more information about the failure
        std::cerr << "Error bits: " << logfile.rdstate() << std::endl;
        logfile.clear(); // Clear error flags
    }
}

auto LogHandler::formatLogMessage(const std::string& message, LogLevel level) -> std::string {
    std::stringstream ss;
    ss << "[" << getCurrentTimestamp() << "] " << logLevelToString(level) << ": " << message;
    return ss.str();
}

auto LogHandler::getCurrentTimestamp() -> std::string {
    auto now = std::time(nullptr);
    #ifdef _WIN32
    std::tm timeInfo;
    localtime_s(&timeInfo, &now);
    auto localTime = &timeInfo;
    #else
    auto localTime = std::localtime(&now);
    #endif

    std::stringstream ss;
    ss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    return ss.str();
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

