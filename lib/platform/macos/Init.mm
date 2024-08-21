#include <ApplicationServices/ApplicationServices.h>
#include <fstream>
#include <iostream>
#include <objc/runtime.h>
#include <unistd.h> // For getpid()

#include <Cocoa/Cocoa.h>

#include "../../../Main.h"
#include "../../ApplicationManager.h"
#include "../../LogHandler.h"
#include "../../PlatformSpecific.h"

__attribute__((constructor))
static void dylib_init() {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        ApplicationManager::getInstance().initialize();
    });
}

pid_t getPID(NSString *processName) {
    ApplicationManager &app = ApplicationManager::getInstance();

    NSArray *runningApps = [[NSWorkspace sharedWorkspace] runningApplications];
    
    for (NSRunningApplication *app in runningApps) {
        NSString *executablePath = [[app executableURL] path];
        if ([executablePath containsString:processName]) {
            return [app processIdentifier];
        }
    }
    return -1;
}

void setAbletonLivePID() {
    ApplicationManager &app = ApplicationManager::getInstance();
    LogHandler::getInstance().info("Init::setAbletonLivePID() called");

    NSString *appName = @"Ableton Live 12 Suite";
    pid_t pidFromApp = getPID(appName);

    if (pidFromApp <= 0) {
        LogHandler::getInstance().info("Failed to get Ableton Live PID");
        return;
    }

    app.setAbletonLivePID(pidFromApp);
    LogHandler::getInstance().info("Ableton Live found with PID: " + std::to_string(app.getAbletonLivePID()));

}

void initializePlatform() {
    ApplicationManager &app = ApplicationManager::getInstance();
    LogHandler::getInstance().info("Init::initializePlatform() called");
  
    setAbletonLivePID();
    app.getEventHandler().setupQuartzEventTap();
}

void runPlatform() {
    [[NSApplication sharedApplication] run];
}
