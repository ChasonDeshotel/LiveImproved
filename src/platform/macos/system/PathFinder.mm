#import <AppKit/AppKit.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

#include "LogGlobal.h"
#include "PID.h"

using PathOptional = std::optional<std::filesystem::path>;

namespace pathutil {
    bool exists(std::filesystem::path path) {
        return std::filesystem::exists(path);
    }

    bool isFile(std::filesystem::path path) {
        if (pathutil::exists(path)) {
            return std::filesystem::is_regular_file(path);
        }
        return false;
    }

    bool isDirectory(std::filesystem::path path) {
        if (pathutil::exists(path)) {
            return std::filesystem::is_directory(path);
        }
        return false;
    }
}

namespace PathFinder {
    std::filesystem::path home() {
        std::filesystem::path homeDir(getenv("HOME"));

        if (!pathutil::isDirectory(homeDir)) {
            throw std::runtime_error("Failed to get home directory");
        }

        return std::filesystem::path(homeDir);
    }

    PathOptional log() {
        std::filesystem::path logFilePath = home()
            / "source" / "ableton" / "LiveImproved" / "log" / "debug.txt"
        ;

        if (!pathutil::exists(logFilePath)) {
            std::cerr << "Log file does not exist: " << logFilePath << ". Attempting to create it.\n";
            try {
                std::filesystem::create_directories(logFilePath.parent_path());
                std::ofstream logFile(logFilePath);
                if (!logFile) {
                    throw std::runtime_error("Failed to create log file");
                }
                logFile.close();
            } catch (const std::exception& e) {
                logger->error("Failed to create log file: {}", std::string(e.what()));
                return std::nullopt;
            }
        }

        if (!pathutil::isFile(logFilePath)) {
            logger->error("Log file is not a regular file: {}", logFilePath.string());
            return std::nullopt;
        }

        return logFilePath;
    }

    PathOptional liveBundle() {
        pid_t pid = PID::getInstance().livePID();
        NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
        NSURL *bundleURL = app.bundleURL;
        std::filesystem::path bundlePath;

        if (!bundleURL) {
            logger->error("unable to get bundle path");
            return std::nullopt;
        }

        bundlePath = [[bundleURL path] UTF8String];

        if (!pathutil::isDirectory(bundlePath)) {
            logger->error("Bundle path does not exist or is not a directory: {}", bundlePath.string());
            return std::nullopt;
        }

        return bundlePath;
    }

    PathOptional liveBinary() {
        auto bundlePath = liveBundle();
        if (!bundlePath) {
            logger->error("Unable to get live bundle path");
            return std::nullopt;
        }

        std::filesystem::path binaryPath = *bundlePath / "Contents" / "MacOS" / "Live";

        if (!pathutil::exists(binaryPath)) {
            logger->error("Binary path does not exist: {}", binaryPath.string());
            return std::nullopt;
        }

        return binaryPath;
    }

    // TODO if no home, attempt to create pipes somewhere else
    std::filesystem::path requestPipe() {
        std::filesystem::path requestPipePath = home()
            / "Music" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "lim_request";
            /// "Documents" / "Ableton" / "User Library"
            /// "Remote Scripts" / "LiveImproved" / "lim_request";

        return requestPipePath;
    }

    std::filesystem::path responsePipe() {
        std::filesystem::path responsePipePath = home()
            / "Music" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "lim_response";

        return responsePipePath;
    }

    PathOptional remoteScripts() {
        std::filesystem::path remoteScriptsPath = home()
            / "Music" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved";
        return remoteScriptsPath;
    }

    // TODO default config and make a config file if !configFile
    PathOptional config() {
        std::filesystem::path configFilePath = home()
            / "Music" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "config.txt";

        if (!pathutil::isFile(configFilePath)) {
            logger->error("Config file does not exist or is not a regular file: {}", configFilePath.string());
            return std::nullopt;
        }

        return configFilePath;
    }

    PathOptional configMenu() {
        std::filesystem::path configMenuPath = home()
            / "Music" / "Ableton" / "User Library"
            / "Remote Scripts" / "LiveImproved" / "config-menu.txt";

        if (!pathutil::isFile(configMenuPath)) {
            logger->error("Menu config file does not exist or is not a regular file: {}", configMenuPath.string());
            return std::nullopt;
        }

        return configMenuPath;
    }

    // TODO default theme if !liveTheme
    // TODO get theme from last access time
    PathOptional liveTheme() {
        auto bundlePath = liveBundle();
        if (!bundlePath) {
            logger->error("Unable to get live bundle path");
            return std::nullopt;
        }

        std::filesystem::path themeFilePath = *bundlePath
            / "Contents" / "App-Resources" / "Themes" / "Default Dark Neutral High.ask";

        if (!pathutil::isFile(themeFilePath)) {
            logger->error("Theme file does not exist or is not a regular file: {}", themeFilePath.string());
            return std::nullopt;
        }

        return themeFilePath;
    }
}
