#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>

#include "EventHandler.h"
#include "../../ApplicationManager.h"

EventHandler::EventHandler(ApplicationManager& appManager)
    : app_(appManager)
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
void EventHandler::setAbletonLivePID() {
    app_.getLogHandler()->info("Init::setAbletonLivePID() called");

    NSString *appName = @"Ableton Live 12 Suite";
    abletonLivePID = getPID(appName);

    if (abletonLivePID <= 0) {
        app_.getLogHandler()->info("Failed to get Ableton Live PID");
        return;
    }

    //app.setAbletonLivePID(pidFromApp);
    app_.getLogHandler()->info("Ableton Live found with PID: " + std::to_string(getAbletonLivePID()));

}

void EventHandler::initialize() {
    app_.getLogHandler()->info("Init::initializePlatform() called");
  
    setAbletonLivePID();
    app_.getEventHandler()->setupQuartzEventTap();
}

void runPlatform() {
    [[NSApplication sharedApplication] run];
}

CGEventRef EventHandler::eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    EventHandler* handler = static_cast<EventHandler*>(refcon);

		pid_t eventPID = (pid_t)CGEventGetIntegerValueField(event, (CGEventField)40);

    if (eventPID == getAbletonLivePID()) {
        handler->app_.getLogHandler()->info("Ableton Live event detected.");

				CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
//				CGEventFlags flags = CGEventGetFlags(event);

        int flags = (int)CGEventGetFlags(event);
        std::string type = kCGEventKeyDown ? "keyDown" : "keyUp";

				handler->app_.getLogHandler()->info("Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

        bool shouldBlock = handler->app_.getActionHandler()->handleKeyEvent(static_cast<int>(keyCode), flags, type);

        return shouldBlock ? NULL : event;
    }

    return event;
}

// move to ActionHandler
//				introspect();
//
//        if (type == kCGEventKeyDown && keyCode == 19 && (CGEventGetFlags(event))) {
//					if (!customAlert || (customAlert && !customAlert.isOpen)) {
//						app_.getLogHandler()->info("showing custom alert");
//						showCustomAlert();
//					}
//				}
//
//        if (keyCode == 53 && type == kCGEventKeyDown && customAlert.searchText.length == 0) {
//            app_.getLogHandler()->info("closing menu.");
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
//            app_.getLogHandler()->info("sending new key event");
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
    app_.getLogHandler()->info("EventHandler::setupQuartEventTap() called");

    if (getAbletonLivePID() == 0) {
        app_.getLogHandler()->info("Ableton Live not found.");
        return;
    }

    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
    CFMachPortRef eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, static_cast<CGEventTapOptions>(0), eventMask, eventTapCallback, this);

    if (!eventTap) {
        app_.getLogHandler()->info("Failed to create event tap.");
        return;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    app_.getLogHandler()->info("Quartz event tap is active!");

    CFRunLoopRun();
}

