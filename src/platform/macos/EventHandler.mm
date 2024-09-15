#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>
#include <string>
#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <chrono>
#include <optional>

#include "EventHandler.h"
#include "ApplicationManager.h"
#include "LogHandler.h"

#include "ActionHandler.h"
#include "PID.h"
#include "WindowManager.h"

// TODO: unsuckify this
// TODO: build out the rest of the map and put it in KeyMapper
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
            UniChar chars[4];
            UniCharCount length;

            CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
            CGEventRef keyEvent = CGEventCreateKeyboardEvent(source, keyCode, false);
            if (keyEvent) {
                CGEventKeyboardGetUnicodeString(keyEvent, sizeof(chars) / sizeof(chars[0]), &length, chars);
                CFRelease(keyEvent);
            }
            CFRelease(source);

            return std::string(chars, chars + length);
    }
}

@class GUISearchBoxWindowController;

EventHandler::EventHandler(WindowManager& windowManager, ActionHandler& actionHandler)
    : windowManager_(windowManager)
    , actionHandler_(actionHandler)
    , log_(LogHandler::getInstance())
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

// should not use this, instead dispatch async
void EventHandler::runPlatform() {
    [[NSApplication sharedApplication] run];
}

void EventHandler::focusLim() {
    EventHandler::focusApplication(PID::getInstance().appPID());
}

void EventHandler::focusLive() {
    EventHandler::focusApplication(PID::getInstance().livePID());
}

void EventHandler::focusApplication(pid_t pid) {
    NSRunningApplication *app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
    
    if (app) {
        // Check if the application is already active
        NSRunningApplication *currentApp = [NSRunningApplication currentApplication];
        if ([currentApp processIdentifier] == pid) {
            LogHandler::getInstance().debug("Application is already in focus: " + std::to_string(pid));
            return;
        }

        LogHandler::getInstance().debug("Bringing app into focus: " + std::to_string(pid));
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

std::optional<std::chrono::steady_clock::time_point> lastRightClickTime;
// TODO: move to config
const int doubleClickThresholdMs = 300;

CGEventRef EventHandler::eventTapCallback(CGEventTapProxy proxy, CGEventType eventType, CGEventRef event, void *refcon) {
    EventHandler* handler = static_cast<EventHandler*>(refcon);

    // 40 is weird. The normal flag didn't work
    pid_t eventPID = (pid_t)CGEventGetIntegerValueField(event, (CGEventField)40);

    // close the search box when clicking in Live
    if (handler->windowManager_.isWindowOpen("SearchBox")) {
        if (eventType == kCGEventLeftMouseDown || eventType == kCGEventRightMouseDown || eventType == kCGEventOtherMouseDown) {
            dispatch_async(dispatch_get_main_queue(), ^{
                pid_t targetPID = (pid_t)CGEventGetIntegerValueField(event, kCGEventTargetUnixProcessID);

                //handler->log_->info("event pid: " + std::to_string(eventPID));
                //handler->log_->info("app pid: " + std::to_string(PID::getInstance().appPID()));
                //handler->log_->info("live pid: " + std::to_string(PID::getInstance().livePID()));
                //handler->log_->info("target pid: " + std::to_string(targetPID));

                CGPoint location = CGEventGetLocation(event);

                NSView *nativeView = reinterpret_cast<NSView *>(handler->windowManager_.getWindowHandle("SearchBox"));
                LogHandler::getInstance().debug("got window handle");

                NSWindow *searchBoxWindow = [nativeView window];

                NSRect appBounds = [searchBoxWindow frame];

                CGPoint mouseLocation = [NSEvent mouseLocation];
                bool isOutsideApp = !NSPointInRect(mouseLocation, appBounds);

                LogHandler::getInstance().debug("Mouse Location: " + std::to_string(mouseLocation.x) + ", " + std::to_string(mouseLocation.y));
                LogHandler::getInstance().debug("App Bounds: " + std::to_string(appBounds.origin.x) + ", " + std::to_string(appBounds.origin.y) + " to " + std::to_string(appBounds.origin.x + appBounds.size.width) + ", " + std::to_string(appBounds.origin.y + appBounds.size.height));

                if (isOutsideApp) {
                    NSRect liveBounds = getLiveBounds();
                    bool isInsideLive = NSPointInRect(mouseLocation, liveBounds);
                    LogHandler::getInstance().debug("Live Bounds: " + std::to_string(liveBounds.origin.x) + ", " + std::to_string(liveBounds.origin.y) + " to " + std::to_string(liveBounds.origin.x + liveBounds.size.width) + ", " + std::to_string(liveBounds.origin.y + liveBounds.size.height));

                    if (isInsideLive) {
                        LogHandler::getInstance().debug("Click is outside app, inside Live, closing window.");
                        handler->windowManager_.closeWindow("SearchBox");
    //                    return NULL;
                    }
                } else {
                    LogHandler::getInstance().debug("Click is inside the window, keeping window open.");
                }
            });
        }
    }

    if (eventPID == PID::getInstance().livePID()) {
        // double-right-click menu
        if (eventType == kCGEventRightMouseDown) {
            handler->log_.debug("Ableton Live right click event detected.");
            //handler->log_->info("right click event");
            auto now = std::chrono::steady_clock::now();

            if (lastRightClickTime.has_value()) {
                auto durationSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRightClickTime.value());
                handler->log_.debug("Duration since last click: " + std::to_string(durationSinceLastClick.count()) + " ms");

                if (durationSinceLastClick.count() != 0 && durationSinceLastClick.count() <= doubleClickThresholdMs) {
                    //handler->log_->info("double right click");
                    dispatch_async(dispatch_get_main_queue(), ^{
                        handler->actionHandler_.handleDoubleRightClick();
                    });
                    return NULL;
                }
            } else {
                // handler->log_->info("First right click detected, skipping double-click check");
            }

            lastRightClickTime = now;
            return event;

        } else if (eventType == kCGEventKeyDown) {
            handler->log_.debug("Ableton Live keydown event detected.");

            CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            CGEventFlags flags = CGEventGetFlags(event);

            std::string keyUpDown = (eventType == kCGEventKeyDown) ? "keyDown" : "keyUp";

            std::string keyString = keyCodeToString(keyCode);

            bool shouldPassEvent = handler->actionHandler_.handleKeyEvent(keyString, flags, keyUpDown);

            return shouldPassEvent ? event : NULL;
        }
    }

    return event;
}

void EventHandler::setupQuartzEventTap() {
    ApplicationManager &app = ApplicationManager::getInstance();
    log_.debug("EventHandler::setupQuartEventTap() called");

    if (PID::getInstance().livePID() == -1) {
        log_.error("EventHandler: Ableton Live not found.");
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
        log_.error("EventHandler: Failed to create event tap.");
        return;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    log_.debug("EventHandler: Quartz event tap is active!");
}

bool isElementFocused(AXUIElementRef element) {
    CFTypeRef focused = NULL;
    AXUIElementCopyAttributeValue(element, kAXFocusedAttribute, &focused);

    if (focused == kCFBooleanTrue) {
        CFRelease(focused);
        return true;
    }

    if (focused) {
        CFRelease(focused);
    }
    return false;
}
