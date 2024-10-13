#import <AppKit/AppKit.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

#include "LogGlobal.h"
#include "PID.h"

namespace PathFinder {
    std::optional<std::filesystem::path> home() {
        std::filesystem::path homeDir(getenv("HOME"));

        if (!std::filesystem::exists(homeDir)) {
            logger->error("Home dir does not exist: " + homeDir.string());
            return std::nullopt;
        }

        if (!std::filesystem::is_directory(homeDir)) {
            logger->error("Home dir is not a directory: " + homeDir.string());
            return std::nullopt;
        }

        return std::filesystem::path(homeDir);
    }

    std::optional<std::filesystem::path> log() {
        auto homeDir = home();
        if (!homeDir) {
            logger->error("Unable to get home directory");
            return std::nullopt;
        }

        std::filesystem::path logFilePath = *homeDir
            / "Scripts" / "Ableton" / "LiveImproved" / "log" / "debug.txt"
        ;

        if (!std::filesystem::exists(logFilePath)) {
            logger->warn("Log file does not exist: " + logFilePath.string() + ". Attempting to create it.");
            try {
                std::filesystem::create_directories(logFilePath.parent_path());
                std::ofstream logFile(logFilePath);
                if (!logFile) {
                    throw std::runtime_error("Failed to create log file");
                }
                logFile.close();
            } catch (const std::exception& e) {
                logger->error("Failed to create log file: " + std::string(e.what()));
                return std::nullopt;
            }
        }

        if (!std::filesystem::is_regular_file(logFilePath)) {
            logger->error("Log file is not a regular file: " + logFilePath.string());
            return std::nullopt;
        }

        return logFilePath;
    }

    std::optional<std::filesystem::path> liveBundle() {
        pid_t pid = PID::getInstance().livePID();
        NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
        NSURL *bundleURL = app.bundleURL;
        std::filesystem::path bundlePath;

        if (!bundleURL) {
            logger->error("unable to get bundle path");
            return std::nullopt;
        }

        bundlePath = [[bundleURL path] UTF8String];

        if (!std::filesystem::exists(bundlePath)) {
            logger->error("Bundle path does not exist: " + bundlePath.string());
            return std::nullopt;
        }

        if (!std::filesystem::is_directory(bundlePath)) {
            logger->error("Bundle path is not a directory: " + bundlePath.string());
            return std::nullopt;
        }

        return bundlePath;
    }

    std::optional<std::filesystem::path> liveBinary() {
        auto bundlePath = liveBundle();
        if (!bundlePath) {
            logger->error("Unable to get live bundle path");
            return std::nullopt;
        }

        std::filesystem::path binaryPath = *bundlePath / "Contents" / "MacOS" / "Live";

        if (!std::filesystem::exists(binaryPath)) {
            logger->error("Binary path does not exist: " + binaryPath.string());
            return std::nullopt;
        }

        return binaryPath;
    }

    // TODO if no home, attempt to create pipes somewhere else
    std::filesystem::path requestPipe() {
        auto homeDir = home();
        if (!homeDir) {
            logger->error("Unable to get home directory");
            throw std::runtime_error("Unable to get home directory");
        }

        std::filesystem::path requestPipePath = *homeDir
            / "Documents" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "lim_request";

        logger->info("\n\npath: " + requestPipePath.generic_string());
        return requestPipePath;
    }

    std::filesystem::path responsePipe() {
        auto homeDir = home();
        if (!homeDir) {
            logger->error("Unable to get home directory");
            throw std::runtime_error("Unable to get home directory");
        }

        std::filesystem::path responsePipePath = *homeDir
            / "Documents" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "lim_response";

        logger->info("\n\npath: " + responsePipePath.generic_string());
        return responsePipePath;
    }

    // TODO default config and make a config file if !configFile
    std::optional<std::filesystem::path> config() {
        auto homeDir = home();
        if (!homeDir) {
            logger->error("Unable to get home directory");
            return std::nullopt;
        }

        std::filesystem::path configFilePath = *homeDir
            / "Documents" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "config.txt";

        if (!std::filesystem::exists(configFilePath)) {
            logger->error("Config file path does not exist: " + configFilePath.string());
            return std::nullopt;
        }

        if (!std::filesystem::is_regular_file(configFilePath)) {
            logger->error("Config file is not a regular file: " + configFilePath.string());
            return std::nullopt;
        }

        return configFilePath;
    }

    std::optional<std::filesystem::path> configMenu() {
        auto homeDir = home();
        if (!homeDir) {
            logger->error("Unable to get home directory");
            return std::nullopt;
        }

        std::filesystem::path configMenuPath = *homeDir
            / "Documents" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "config-menu.txt";

        if (!std::filesystem::exists(configMenuPath)) {
            logger->error("Menu config file path does not exist: " + configMenuPath.string());
            return std::nullopt;
        }

        if (!std::filesystem::is_regular_file(configMenuPath)) {
            logger->error("Menu config file is not a regular file: " + configMenuPath.string());
            return std::nullopt;
        }

        return configMenuPath;
    }

    // TODO default theme if !liveTheme
    std::optional<std::filesystem::path> liveTheme() {
        auto bundlePath = liveBundle();
        if (!bundlePath) {
            logger->error("Unable to get live bundle path");
            return std::nullopt;
        }

        std::filesystem::path themeFilePath = *bundlePath
            / "Contents" / "App-Resources" / "Themes" / "Default Dark Neutral High.ask";

        if (!std::filesystem::exists(themeFilePath)) {
            logger->error("Theme file path does not exist: " + themeFilePath.string());
            return std::nullopt;
        }

        if (!std::filesystem::is_regular_file(themeFilePath)) {
            logger->error("Theme file is not a regular file: " + themeFilePath.string());
            return std::nullopt;
        }

        return themeFilePath;
    }
}
