#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>

#include "ApplicationManager.h"
#include "EventHandler.h"
#include "LogHandler.h"
#include "PID.h"

std::string keyCodeToString(CGKeyCode keyCode) {
    // Handle special keys
    switch (keyCode) {
        // Function keys
        case 122: return "F1";        // kVK_F1
        case 120: return "F2";        // kVK_F2
        case 99: return "F3";         // kVK_F3
        case 118: return "F4";        // kVK_F4
        case 96: return "F5";         // kVK_F5
        case 97: return "F6";         // kVK_F6
        case 98: return "F7";         // kVK_F7
        case 100: return "F8";        // kVK_F8
        case 101: return "F9";        // kVK_F9
        case 103: return "F10";       // kVK_F10
        case 105: return "F11";       // kVK_F11
        case 107: return "F12";       // kVK_F12

        // Arrow keys
        case 123: return "Left Arrow";   // kVK_LeftArrow
        case 124: return "Right Arrow";  // kVK_RightArrow
        case 125: return "Down Arrow";   // kVK_DownArrow
        case 126: return "Up Arrow";     // kVK_UpArrow

        // Other special keys
        case 53: return "Escape";    // kVK_Escape
        case 48: return "Tab";       // kVK_Tab
        case 49: return "Space";     // kVK_Space
        case 36: return "Return";    // kVK_Return
        case 51: return "Delete";    // kVK_Delete
        case 115: return "Home";     // kVK_Home
        case 119: return "End";      // kVK_End
        case 116: return "Page Up";  // kVK_PageUp
        case 121: return "Page Down";// kVK_PageDown

        // Other keys
        case 86: return "+";         // kVK_ANSI_Equal (Shift + =)
        case 27: return "-";         // kVK_ANSI_Minus
        //case 30: return "=";         // kVK_ANSI_Equal
        case 50: return "`";         // kVK_ANSI_Grave
        case 33: return "[";         // kVK_ANSI_LeftBracket
        case 30: return "]";         // kVK_ANSI_RightBracket
        case 42: return "\\";        // kVK_ANSI_Backslash
        
        default:
            // Create a source and event to get the Unicode string
            UniChar chars[4];
            UniCharCount length;

            CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
            CGEventRef keyEvent = CGEventCreateKeyboardEvent(source, keyCode, false);
            if (keyEvent) {
                CGEventKeyboardGetUnicodeString(keyEvent, sizeof(chars) / sizeof(chars[0]), &length, chars);
                CFRelease(keyEvent);
            }
            CFRelease(source);

            // Convert to std::string
            return std::string(chars, chars + length);
    }
}


@class GUISearchBoxWindowController;

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

void EventHandler::focusApplication(pid_t pid) {
    NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
            
    if (app) {
        LogHandler::getInstance().info("bringing app into focus: " + std::to_string(PID::getInstance().appPID()));

        [app activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];
    }
}

NSRect getLiveBounds() {
    NSRect appBounds = NSMakeRect(0, 0, 0, 0);

    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
    CFIndex count = CFArrayGetCount(windowList);

    for (CFIndex i = 0; i < count; i++) {
        CFDictionaryRef windowInfo = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

        CFNumberRef windowPID;
        windowPID = (CFNumberRef)CFDictionaryGetValue(windowInfo, kCGWindowOwnerPID);

        pid_t windowPidValue;
        CFNumberGetValue(windowPID, kCFNumberIntType, &windowPidValue);

        if (windowPidValue == PID::getInstance().livePID()) {
            CFDictionaryRef boundsDict = (CFDictionaryRef)CFDictionaryGetValue(windowInfo, kCGWindowBounds);
            CGRect bounds;
            CGRectMakeWithDictionaryRepresentation(boundsDict, &bounds);

            appBounds = NSRectFromCGRect(bounds);
            break;
        }
    }

    CFRelease(windowList);

    return appBounds;
}

ERect EventHandler::getLiveBoundsRect() {
    NSRect appBounds = getLiveBounds();
    ERect rect;
    rect.x = static_cast<int>(appBounds.origin.x);
    rect.y = static_cast<int>(appBounds.origin.y);
    rect.width = static_cast<int>(appBounds.size.width);
    rect.height = static_cast<int>(appBounds.size.height);
    return rect;
}

CGEventRef EventHandler::eventTapCallback(CGEventTapProxy proxy, CGEventType eventType, CGEventRef event, void *refcon) {
    EventHandler* handler = static_cast<EventHandler*>(refcon);

		pid_t eventPID = (pid_t)CGEventGetIntegerValueField(event, (CGEventField)40);
    CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    CGEventFlags flags = CGEventGetFlags(event);

    bool isShiftPressed   = (flags & kCGEventFlagMaskShift)     != 0;
    bool isControlPressed = (flags & kCGEventFlagMaskControl)   != 0;
    bool isAltPressed     = (flags & kCGEventFlagMaskAlternate) != 0;
    bool isCommandPressed = (flags & kCGEventFlagMaskCommand)   != 0;

    if (handler->app_.getGUISearchBox()->isOpen()) {
        if (eventType == kCGEventLeftMouseUp || eventType == kCGEventRightMouseUp || eventType == kCGEventOtherMouseUp) {
            pid_t targetPID = (pid_t)CGEventGetIntegerValueField(event, kCGEventTargetUnixProcessID);

            handler->log_->info("event pid: " + std::to_string(eventPID));
            handler->log_->info("app pid: " + std::to_string(PID::getInstance().appPID()));
            handler->log_->info("live pid: " + std::to_string(PID::getInstance().livePID()));
            handler->log_->info("target pid: " + std::to_string(targetPID));

            CGPoint location = CGEventGetLocation(event);

            NSView *nativeView = reinterpret_cast<NSView *>(handler->app_.getGUISearchBox()->getQtWidget()->winId());
            NSWindow *searchBoxWindow = [nativeView window];

            NSRect appBounds = [searchBoxWindow frame];

            CGPoint mouseLocation = [NSEvent mouseLocation];
            bool isOutsideApp = !NSPointInRect(mouseLocation, appBounds);

            LogHandler::getInstance().info("Mouse Location: " + std::to_string(mouseLocation.x) + ", " + std::to_string(mouseLocation.y));
            LogHandler::getInstance().info("App Bounds: " + std::to_string(appBounds.origin.x) + ", " + std::to_string(appBounds.origin.y) + " to " + std::to_string(appBounds.origin.x + appBounds.size.width) + ", " + std::to_string(appBounds.origin.y + appBounds.size.height));

            if (isOutsideApp) {
                NSRect liveBounds = getLiveBounds();
                bool isInsideLive = NSPointInRect(mouseLocation, liveBounds);
                LogHandler::getInstance().info("Live Bounds: " + std::to_string(liveBounds.origin.x) + ", " + std::to_string(liveBounds.origin.y) + " to " + std::to_string(liveBounds.origin.x + liveBounds.size.width) + ", " + std::to_string(liveBounds.origin.y + liveBounds.size.height));

                if (isInsideLive) {
                    LogHandler::getInstance().info("Click is outside app, inside Live, closing window.");
//                    handler->app_.getGUISearchBox()->closeSearchBox();
//                    return NULL;
                }
            } else {
                LogHandler::getInstance().info("Click is inside the window, keeping window open.");
            }

        }
    }

//    } else if (eventPID == PID::getInstance().livePID()) {
    if (eventPID == PID::getInstance().livePID()) {
        handler->log_->info("Ableton Live event detected.");

        int flagsInt = (int)CGEventGetFlags(event);
        CGEventFlags flags = CGEventGetFlags(event);
        std::string keyUpDown = (eventType == kCGEventKeyDown) ? "keyDown" : "keyUp";

        std::string keyString = keyCodeToString(keyCode);
        LogHandler::getInstance().info("translated key: " + keyString);
        bool shouldPassEvent = handler->app_.getActionHandler()->handleKeyEvent(keyString, flags, keyUpDown);

        if (keyUpDown == "keyDown") {
            handler->log_->info("Key event: " + keyUpDown + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flagsInt) + " should pass: " + std::to_string(shouldPassEvent));
        }

        return shouldPassEvent ? event : NULL;
    }

    return event;
}

void EventHandler::setupQuartzEventTap() {
    ApplicationManager &app = ApplicationManager::getInstance();
    log_->info("EventHandler::setupQuartEventTap() called");

    if (PID::getInstance().livePID() == -1) {
        log_->info("Ableton Live not found.");
        return;
    }

    CGEventMask eventMask = (1 << kCGEventKeyDown)
                            | (1 << kCGEventKeyUp)
                            | (1 << kCGEventLeftMouseDown)
                            | (1 << kCGEventRightMouseDown)
                            | (1 << kCGEventOtherMouseDown)
                            | (1 << kCGEventLeftMouseUp)
                            | (1 << kCGEventRightMouseUp)
                            | (1 << kCGEventOtherMouseUp)
                            ;
    CFMachPortRef eventTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, static_cast<CGEventTapOptions>(0), eventMask, eventTapCallback, this);

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

