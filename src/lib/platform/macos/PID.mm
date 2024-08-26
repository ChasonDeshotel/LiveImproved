#include <ApplicationServices/ApplicationServices.h>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>
#include <iostream>
#include <fstream>

#include "ApplicationManager.h"
#include "PID.h"

PID::PID(ApplicationManager& appManager)
    : app_(appManager)
    , log_(appManager.getLogHandler())
    , abletonLivePID(-1)
{}

PID::~PID() {}

pid_t PID::findByName(std::string processName) {
    log_->info("PID::findByName() called");

    if (abletonLivePID != -1) {
      log_->info("PID::findByName() - returning cached result");
      return abletonLivePID;
    }

    log_->info("PID::findByName() - searching running applications");
    NSString *nsProcessName = [NSString stringWithUTF8String:processName.c_str()];
    NSArray *runningApps = [[NSWorkspace sharedWorkspace] runningApplications];
    
    for (NSRunningApplication *app in runningApps) {
        NSString *executablePath = [[app executableURL] path];
        if ([executablePath containsString:nsProcessName]) {
            pid_t PID = [app processIdentifier];
            log_->info("Ableton Live found with PID: " + std::to_string(PID));
            abletonLivePID = PID;
            return PID;
        }
    }

    log_->info("Failed to get Ableton Live PID");
    return -1;
}

// TODO: consider looping here, the app is useless without the PID
pid_t PID::livePID() {
    if (abletonLivePID != -1) {
      //log_->info("PID::livePID() - returning cached result");
      return abletonLivePID;
    }

    log_->info("PID::livePID() - finding by name");
    std::string appName = "Ableton Live 12 Suite";

    return findByName(appName);
}


PID* PID::init() {
    log_->info("PID::Init() called");
    std::string appName = "Ableton Live 12 Suite";

    #ifdef INJECTED_LIBRARY
      findByName(appName);
    #else
        #include <unistd.h>
        while (livePID() == -1) {
            LogHandler::getInstance().info("Application not found, retrying...");
            livePID();
            sleep(1);
        }
    #endif
  
    return this;
}
