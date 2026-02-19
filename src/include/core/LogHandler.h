#pragma once
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <fmt/format.h>

#if !defined(TEST_BUILD) && !defined(NO_JUCE)
#include <JuceHeader.h>
#endif

#include "ILogger.h"
#include "LogHandler.h"
#include "LogSink.h"
#include "FileSink.h"

#if !defined(TEST_BUILD) && !defined(NO_JUCE)
#include "JUCESink.h"
#endif

namespace fs = std::filesystem;

class LogHandler : public ILogger {
public:
    ~LogHandler() override = default;

    auto addSink(const std::shared_ptr<LogSink>& sink) -> void override {
        if (sink && sink->isAvailable()) {
            std::lock_guard lock(logMutex_);
            sinks_.push_back(sink);
        }
    }

    auto setLogPath(const std::string& path) -> void override {
        std::lock_guard lock(logMutex_);
        logPath_ = path;

        // If we already have a file sink, update it or add a new one
        if (auto fileSink = std::make_shared<FileLogSink>(path); fileSink->isAvailable()) {
            addSink(fileSink);
        }
    }

    auto toString(const LogCategory category) -> std::string override {
        switch (category) {
            default: return "UNINIMPLEMENTED";
        }

    }
    auto toString(const LogLevel level) -> std::string override {
        switch (level) {
            case LogLevel::LOG_TRACE: return "TRACE";
            case LogLevel::LOG_DEBUG: return "DEBUG";
            case LogLevel::LOG_INFO: return "INFO";
            case LogLevel::LOG_WARN: return "WARN";
            case LogLevel::LOG_ERROR: return "ERROR";
            case LogLevel::LOG_FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    unsigned int fmtSafeOffset(unsigned long x) { return static_cast<unsigned int>(x); }
    unsigned int fmtSafeOffset64(uint64_t x) { return static_cast<unsigned int>(x); }
    unsigned int fmtSafeOffset32(uint32_t x) { return static_cast<unsigned int>(x); }
    unsigned int fmtSafeOffset(int x) { return static_cast<unsigned int>(x); }
    unsigned int fmtSafeOffset(unsigned int x) { return x; }

    auto setLogLevel(const LogLevel level) -> void override {
        std::lock_guard lock(logMutex_);
        currentLogLevel_ = level;
    }

    //auto setCategoryEnabled(const LogCategory category, const bool enabled) -> void {
    //    std::lock_guard lock(logMutex);
    //    enabledCategories[category] = enabled;

    //    // Log this change if possible
    //    if (enabled) {
    //        log("Enabled logging for category: " + toString(category), LogCategory::CORE, LogLevel::LOG_INFO);
    //    } else {
    //        log("Disabled logging for category: " + toString(category), LogCategory::CORE, LogLevel::LOG_INFO);
    //    }
    //}

  private:
    void logImpl(std::string_view message, LogLevel level) override {
        std::lock_guard lock(logMutex_);
        std::string formattedMessage = fmt::format("[{:>5}] {}", toString(level), message);

        for (const auto& sink : sinks_) {
            if (sink->isAvailable()) {
                sink->write(formattedMessage);
                sink->flush();
            }
        }
    }

    std::ofstream logfile_;
    std::filesystem::path logPath_;
    LogLevel currentLogLevel_{ LogLevel::LOG_DEBUG };
    std::unordered_map<LogCategory, bool> enabledCategories_;
    std::mutex logMutex_;
    static std::mutex consoleMutex_;
    std::vector<std::shared_ptr<LogSink>> sinks_;
};