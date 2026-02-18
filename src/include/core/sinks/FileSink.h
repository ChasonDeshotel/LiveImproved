#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>

#include "LogSink.h"

class FileLogSink final : public LogSink {
public:
    explicit FileLogSink(const std::filesystem::path& path, bool append = true)
      : logFilePath(path) {

        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        logFile.open(path, append ? std::ios::app : std::ios::trunc);
    }

    ~FileLogSink() override {
        if (logFile && logFile.is_open()) {
            logFile.close();
        }
    }
    FileLogSink(FileLogSink&&) = delete;
    FileLogSink(const FileLogSink&) = delete;
    auto operator=(FileLogSink&&) -> FileLogSink& = delete;
    auto operator=(const FileLogSink&) -> FileLogSink& = delete;

    void write(const std::string_view formattedMessage) override {
        std::lock_guard lock(fileMutex);
        if (logFile.is_open()) {
            logFile << formattedMessage << std::endl;
        }
    }

    [[nodiscard]] auto isAvailable() const -> bool override { return logFile.is_open() && logFile.good(); }

    void flush() override {
        std::lock_guard lock(fileMutex);
        if (logFile.is_open()) {
            logFile.flush();
        }
    }

private:
    std::filesystem::path logFilePath;
    std::ofstream logFile;
    std::mutex fileMutex;
};