#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>

#include "../../../Main.h"
#include "../../ApplicationManager.h"
#include "../../Log.h"
#include "../../PlatformSpecific.h"
#include "../../ActionHandler.h"

CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    ApplicationManager &app = ApplicationManager::getInstance();

		pid_t eventPID = (pid_t)CGEventGetIntegerValueField(event, (CGEventField)40);

    if (eventPID == app.getAbletonLivePID()) {
        Log::logToFile("Ableton Live event detected.");

				CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
//				CGEventFlags flags = CGEventGetFlags(event);

        int flags = (int)CGEventGetFlags(event);
        std::string type = kCGEventKeyDown ? "KeyDown" : "KeyUp";

				Log::logToFile("Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

        bool shouldBlock = ActionHandler::getInstance().handleKeyEvent(static_cast<int>(keyCode), flags, type);

        return shouldBlock ? NULL : event;
    }

    return event;
}

//
//				introspect();
//
//        if (type == kCGEventKeyDown && keyCode == 19 && (CGEventGetFlags(event))) {
//					if (!customAlert || (customAlert && !customAlert.isOpen)) {
//						Log::logToFile("showing custom alert");
//						showCustomAlert();
//					}
//				}
//
//        if (keyCode == 53 && type == kCGEventKeyDown && customAlert.searchText.length == 0) {
//            Log::logToFile("closing menu.");
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
//            Log::logToFile("sending new key event");
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

void setupQuartzEventTap() {
    ApplicationManager &app = ApplicationManager::getInstance();
    if (app.getAbletonLivePID() == 0) {
        Log::logToFile("Ableton Live not found.");
        return;
    }

    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
    CFMachPortRef eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, static_cast<CGEventTapOptions>(0), eventMask, eventTapCallback, NULL);

    if (!eventTap) {
        Log::logToFile("Failed to create event tap.");
        return;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    Log::logToFile("Quartz event tap is active!");

    CFRunLoopRun();
}

void runPlatform() {
    // Start the Cocoa event loop or other macOS-specific runtime
    [[NSApplication sharedApplication] run];
}
