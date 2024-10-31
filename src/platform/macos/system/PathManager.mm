#import <AppKit/AppKit.h>

#include <fstream>
#include <iostream>

#include "PathManager.h"
#include "LogGlobal.h"
#include "PID.h"

namespace fs = std::filesystem;
using Path = std::filesystem::path;

PathManager::PathManager() {
    // Add default search paths
    searchPaths.emplace_back(getenv("HOME"));
}

// accounts for symlinks
auto PathManager::isValidFile(const fs::path& path) const -> bool {
    fs::file_status status = fs::symlink_status(path);
    return fs::is_regular_file(status);
}

// accounts for symlinks
auto PathManager::isValidDir(const fs::path& path) const -> bool {
    fs::file_status status = fs::symlink_status(path);
    return fs::is_directory(status);
}

auto PathManager::findPath(const std::string& key) const -> Path {
    if (cachedPaths.find(key) != cachedPaths.end()) {
        return cachedPaths[key];
    }

    for (const auto& searchPath : searchPaths) {
        Path fullPath = searchPath / key;
        if (std::filesystem::exists(fullPath)) {
            cachedPaths[key] = fullPath;
            return fullPath;
        }
    }

//    if (fallbackPaths.find(key) != fallbackPaths.end()) {
//        return fallbackPaths[key];
//    }

    throw std::runtime_error("Path not found: " + key);
}

auto PathManager::documents() const -> Path {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    if (paths.count > 0) {
        NSString* documentsDirectory = paths.firstObject;
        Path docPath([documentsDirectory UTF8String]);

        if (!isValidDir(docPath)) {
            throw std::runtime_error("Documents directory does not exist or is not a directory: " + docPath.string());
        }

        return docPath;
    }

    throw std::runtime_error("Unable to locate Documents directory");
}

auto PathManager::music() const -> Path {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES);
    if (paths.count > 0) {
        NSString* musicDirectory = paths.firstObject;
        Path musicPath([musicDirectory UTF8String]);

        if (!isValidDir(musicPath)) {
            throw std::runtime_error("Music directory does not exist or is not a directory: " + musicPath.string());
        }

        return musicPath;
    }

    throw std::runtime_error("Unable to locate Music directory");
}

auto PathManager::config() const -> Path {
    Path configFile = limRemoteScript() / "config.txt";

    if (!isValidFile(configFile)) {
        throw std::runtime_error("Config doesn't exist or is not a file!");
    }

    return configFile;
}

auto PathManager::configMenu() const -> Path {
    Path configFile = limRemoteScript() / "config-menu.txt";

    if (!isValidFile(configFile)) {
        throw std::runtime_error("Menu config doesn't exist or is not a file!");
    }

    return configFile;
}

void PathManager::addSearchPath(const std::filesystem::path& path) {
    searchPaths.push_back(path);
}

void PathManager::setFallbackPath(const std::string& key, const std::filesystem::path& path) {
    fallbackPaths[key] = path;
}

auto PathManager::home() const -> Path {
    const char* homeEnv = getenv("HOME");
    if (!isValidDir(homeEnv)) {
        throw std::runtime_error("HOME environment variable not set");
    }
    
    Path homePath(homeEnv);
    if (!isValidDir(homePath)) {
        throw std::runtime_error("Home directory does not exist or is not a directory: " + homePath.string());
    }
    
    return homePath;
}

auto PathManager::log() const -> Path {
    return home() / "Scripts" / "Ableton" / "LiveImproved" / "log" / "debug.txt";
}

auto PathManager::liveBundle() const -> Path {
    pid_t pid = PID::getInstance().livePID();
    if (pid == -1) {
        logger->warn("Live must be running to get bundleURL");
    }
    NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
    NSURL *bundleURL = app.bundleURL;
    
    if (!bundleURL) {
        throw std::runtime_error("Unable to get bundle path");
    }
    
    Path bundlePath = [[bundleURL path] UTF8String];
    
    if (!isValidDir(bundlePath)) {
        throw std::runtime_error("Bundle path does not exist or is not a directory: " + bundlePath.string());
    }
    
    return bundlePath;
}

auto PathManager::liveBinary() const -> Path {
    Path binaryPath = liveBundle() / "Contents" / "MacOS" / "Live";
    
    if (!isValidFile(binaryPath)) {
        throw std::runtime_error("Binary path does not exist: " + binaryPath.string());
    }
    
    return binaryPath;
}

auto PathManager::liveThemes() const -> Path {
    Path themesPath = liveBundle() / "Contents" / "App-Resources" / "Themes";
    
    if (!isValidDir(themesPath)) {
        throw std::runtime_error("Themes directory does not exist: " + themesPath.string());
    }
    
    return themesPath;
}

auto PathManager::liveTheme() const -> Path {
    Path themeFilePath = liveThemes() / "Default Dark Neutral High.ask";
    
    if (!isValidFile(themeFilePath)) {
        throw std::runtime_error("Theme file does not exist or is not a regular file: " + themeFilePath.string());
    }
    
    return themeFilePath;
}

auto PathManager::remoteScripts() const -> Path {
    Path remoteScriptsInDocuments = documents() / "Ableton" / "User Library" / "Remote Scripts";
    Path remoteScriptsInMusic = music() / "Ableton" / "User Library" / "Remote Scripts";

    if (isValidDir(remoteScriptsInDocuments)) {
        return remoteScriptsInDocuments;
    } else if (isValidDir(remoteScriptsInMusic)) {
        return remoteScriptsInMusic;
    } else {
        throw std::runtime_error("Ableton Live MIDI Remote Scripts directory does not exist or is not a directory in either Documents or Music folders");
    }
}

auto PathManager::limRemoteScript() const -> Path {
    Path limRemoteScript = remoteScripts() / "LiveImproved";

    if (!isValidDir(limRemoteScript)) {
        throw std::runtime_error("LiveImproved MIDI Remote Script directory does not exist or is not a directory: " + limRemoteScript.string());
    }

    return limRemoteScript;
}

auto PathManager::lesConfig() const -> Path {
    Path lesConfig = home() / ".les" / "menuconfig.ini";

    if (!isValidFile(lesConfig)) {
        throw std::runtime_error("Live Enhancement Suite config file is not found or is invalid: " + lesConfig.string());
    }

    return lesConfig;
}

auto PathManager::requestPipe() const -> Path {
    return limRemoteScript() / "lim_request";
}

auto PathManager::responsePipe() const -> Path {
    return limRemoteScript() / "lim_response";
}
