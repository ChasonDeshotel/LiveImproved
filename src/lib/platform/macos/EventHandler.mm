#include <ApplicationServices/ApplicationServices.h>
#include <string>
#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>

#include "../../ApplicationManager.h"
#include "EventHandler.h"

EventHandler::EventHandler(ApplicationManager& appManager)
    : app_(appManager)
    , log_(appManager.getLogHandler())
    , eventTap(nullptr)
    , runLoopSource(nullptr)
{}

EventHandler::~EventHandler() {
    if (eventTap) {
        CFRelease(eventTap);
    }
    if (runLoopSource) {
        CFRelease(runLoopSource);
    }
}

void EventHandler::init() {
    log_->info("EventHandler::Init() called");
  
    app_.getEventHandler()->setupQuartzEventTap();

}

// should not use this, instead dispatch async
void EventHandler::runPlatform() {
    [[NSApplication sharedApplication] run];
}

CGEventRef EventHandler::eventTapCallback(CGEventTapProxy proxy, CGEventType eventType, CGEventRef event, void *refcon) {
    EventHandler* handler = static_cast<EventHandler*>(refcon);

		pid_t eventPID = (pid_t)CGEventGetIntegerValueField(event, (CGEventField)40);

    if (eventPID == handler->app_.getPID()) {
//        handler->log_->info("Ableton Live event detected.");

				CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
//				CGEventFlags flags = CGEventGetFlags(event);

        int flags = (int)CGEventGetFlags(event);
        std::string keyUpDown = (eventType == kCGEventKeyDown) ? "keyDown" : "keyUp";
        bool shouldPassEvent = handler->app_.getActionHandler()->handleKeyEvent(static_cast<int>(keyCode), flags, keyUpDown);

        if (keyUpDown == "keyDown") {
            handler->log_->info("Key event: " + keyUpDown + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags) + " should pass: " + std::to_string(shouldPassEvent));
        }

        return shouldPassEvent ? event : NULL;
    }

    return event;
}

void EventHandler::setupQuartzEventTap() {
    ApplicationManager &app = ApplicationManager::getInstance();
    log_->info("EventHandler::setupQuartEventTap() called");

    if (app_.getPID() == -1) {
        log_->info("Ableton Live not found.");
        return;
    }

    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
    CFMachPortRef eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, static_cast<CGEventTapOptions>(0), eventMask, eventTapCallback, this);

    if (!eventTap) {
        log_->info("Failed to create event tap.");
        return;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    log_->info("Quartz event tap is active!");

    // blocking mode
    //CFRunLoopRun();
    
    // non-blocking
    //while (true) {
    //    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);  // 0.1-second timeout

    //    app.getIPC()->init();
    //    // Additional non-blocking tasks can be done here
    //}
}

