#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>

#include "EventHandler.h"
#include "../../ApplicationManager.h"
#include "../../LogHandler.h"

EventHandler::EventHandler() : 
    log(LogHandler::getInstance())
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

pid_t abletonLivePID;

pid_t getPID(NSString *processName) {
//    ApplicationManager &app = ApplicationManager::getInstance();

    NSArray *runningApps = [[NSWorkspace sharedWorkspace] runningApplications];
    
    for (NSRunningApplication *app in runningApps) {
        NSString *executablePath = [[app executableURL] path];
        if ([executablePath containsString:processName]) {
            return [app processIdentifier];
        }
    }
    return -1;
}

pid_t getAbletonLivePID() {
    return abletonLivePID;
}

// should be on init
void setAbletonLivePID() {
//    ApplicationManager &app = ApplicationManager::getInstance();
    LogHandler::getInstance().info("Init::setAbletonLivePID() called");

    NSString *appName = @"Ableton Live 12 Suite";
    abletonLivePID = getPID(appName);

    if (abletonLivePID <= 0) {
        LogHandler::getInstance().info("Failed to get Ableton Live PID");
        return;
    }

    //app.setAbletonLivePID(pidFromApp);
    LogHandler::getInstance().info("Ableton Live found with PID: " + std::to_string(getAbletonLivePID()));

}

void EventHandler::initialize() {
    ApplicationManager &app = ApplicationManager::getInstance();
    LogHandler::getInstance().info("Init::initializePlatform() called");
  
    setAbletonLivePID();
    app.getEventHandler().setupQuartzEventTap();
}

void runPlatform() {
    [[NSApplication sharedApplication] run];
}

CGEventRef EventHandler::eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    // because callback
    // and it was getting crashy
    // with more cute implementations
    ApplicationManager &app = ApplicationManager::getInstance();
    LogHandler &log = LogHandler::getInstance();

		pid_t eventPID = (pid_t)CGEventGetIntegerValueField(event, (CGEventField)40);

    if (eventPID == getAbletonLivePID()) {
        log.info("Ableton Live event detected.");

				CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
//				CGEventFlags flags = CGEventGetFlags(event);

        int flags = (int)CGEventGetFlags(event);
        std::string type = kCGEventKeyDown ? "keyDown" : "keyUp";

				log.info("Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

//        bool shouldBlock = app.getActionHandler().handleKeyEvent(static_cast<int>(keyCode), flags, type);
        bool shouldBlock = false;

        return shouldBlock ? NULL : event;
    }

    return event;
}

// move to ActionHandler
//				introspect();
//
//        if (type == kCGEventKeyDown && keyCode == 19 && (CGEventGetFlags(event))) {
//					if (!customAlert || (customAlert && !customAlert.isOpen)) {
//						log.info("showing custom alert");
//						showCustomAlert();
//					}
//				}
//
//        if (keyCode == 53 && type == kCGEventKeyDown && customAlert.searchText.length == 0) {
//            log.info("closing menu.");
//            if (customAlert && customAlert.isOpen) {
//								[customAlert closeAlert];
//                customAlert = nullptr;
//								return NULL;
//						}
//        }
//
//				// when the menu is open, do not send keypresses to Live
//        // or it activates your hotkeys
//				if (customAlert && customAlert.isOpen) {
//						return event;
//				}
//
//        if (type == kCGEventKeyDown && keyCode == 18 && (CGEventGetFlags(event))) {
//            log.info("sending new key event");
//
//            CGEventRef newKeyDownEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)46, true); // 37 is the keyCode for 'L'
//						CGEventFlags flags = kCGEventFlagMaskCommand | kCGEventFlagMaskShift;
//            CGEventSetFlags(newKeyDownEvent, flags);
//            CGEventPost(kCGAnnotatedSessionEventTap, newKeyDownEvent);
//
//            CGEventRef newKeyUpEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)46, false); // Key up event
//						CGEventSetFlags(newKeyDownEvent, flags);
//            CGEventPost(kCGAnnotatedSessionEventTap, newKeyUpEvent);
//
//            CFRelease(newKeyDownEvent);
//            CFRelease(newKeyUpEvent);
//
//            return NULL; // block the origianl event
//        }
//		}
//
//		return event;
//}

void EventHandler::setupQuartzEventTap() {
    ApplicationManager &app = ApplicationManager::getInstance();
    log.info("EventHandler::setupQuartEventTap() called");

    if (getAbletonLivePID() == 0) {
        log.info("Ableton Live not found.");
        return;
    }

    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
    CFMachPortRef eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, static_cast<CGEventTapOptions>(0), eventMask, eventTapCallback, NULL);

    if (!eventTap) {
        log.info("Failed to create event tap.");
        return;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    log.info("Quartz event tap is active!");

    CFRunLoopRun();
}

