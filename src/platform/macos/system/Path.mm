#import <AppKit/AppKit.h>
#include <filesystem>

#include "LogGlobal.h"
#include "PID.h"

namespace Path {
    std::filesystem::path liveBinaryPath() {
        pid_t pid = PID::getInstance().livePID();
        NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
        NSURL *bundleURL = app.bundleURL;

        if (bundleURL) {
            NSString *nsStringPath = [[bundleURL path] stringByAppendingPathComponent:@"Contents/MacOS"];
            const char* utf8Path = [nsStringPath UTF8String];
            std::filesystem::path binaryPath(utf8Path);

            return binaryPath;
        } else {
            logger->error("unable to get binary path");
        }
        return "";
    }
}
