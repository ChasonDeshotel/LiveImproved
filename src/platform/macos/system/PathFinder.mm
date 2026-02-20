#import <AppKit/AppKit.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <libproc.h>

#include "LogGlobal.h"
#include "PID.h"

using PathOptional = std::optional<std::filesystem::path>;

static inline NSString* toNS(const std::string& s) {
    return [NSString stringWithUTF8String:s.c_str()];
}

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

    NSString* getLivePrefsDir() {
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
        auto pid = PID::getInstance().findLivePID();
        proc_pidpath(pid, pathbuf, sizeof(pathbuf));

        std::filesystem::path execPath = pathbuf;
        auto appPath = toNS(execPath.parent_path().parent_path().parent_path());

        auto* liveBundle = [NSBundle bundleWithPath:appPath];
        NSString* version = [liveBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
        std::string shortVersion = [[version componentsSeparatedByString:@" "].firstObject UTF8String];
        auto latestVersionPathStr = std::string("~/Library/Preferences/Ableton/Live ") + shortVersion;

        auto nsPathStr = toNS(latestVersionPathStr);
        nsPathStr = [nsPathStr stringByExpandingTildeInPath];
        return nsPathStr;
    }

    std::filesystem::path getRemoteScriptsPath() {
        NSString* pathStr = getLivePrefsDir();
        std::string pathStdStr = [pathStr UTF8String];
        NSString* cfgPath = [pathStr stringByAppendingPathComponent:@"Library.cfg"];
        NSData* data = [NSData dataWithContentsOfFile:cfgPath];
        NSXMLDocument* doc = [[NSXMLDocument alloc] initWithData:data options:0 error:nil];
        NSArray* nodes = [doc nodesForXPath:@"//UserLibrary/LibraryProject/ProjectPath/@Value" error:nil];
        std::string projectPath = [[[[nodes firstObject] stringValue] stringByAppendingPathComponent:@"User Library/Remote Scripts/LiveImproved"] UTF8String];
        return projectPath;
    }

    NSString* getLIMPrefsDirNS() {
        return [@"~/Library/Preferences/LiveImproved" stringByExpandingTildeInPath];
    }

    std::filesystem::path getLIMPrefsDir() {
        return [getLIMPrefsDirNS() UTF8String];
    }

    NSString* getLogPathNS() {
        return [@"~/Library/Logs/LiveImproved" stringByExpandingTildeInPath];
    }

    PathOptional log() {
        std::filesystem::path logPath = [PathFinder::getLogPathNS() UTF8String];
        return logPath / "debug.txt";
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
        auto configFilePath = getLIMPrefsDir() / "config.yaml";

        if (!pathutil::isFile(configFilePath)) {
            logger->error("Config file does not exist or is not a regular file: {}", configFilePath.string());
            return std::nullopt;
        }

        return configFilePath;
    }

    PathOptional configMenu() {
        auto configMenuPath = getLIMPrefsDir() / "config-menu.yaml";

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
