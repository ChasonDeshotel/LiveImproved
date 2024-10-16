#import <ApplicationServices/ApplicationServices.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <functional>
#include <iostream>
#include <memory>
#include <objc/runtime.h>
#include <optional>
#include <string>
#include <unistd.h>

#include "LogGlobal.h"
#include "MacUtils.h"

#include "EventHandler.h"
#include "IActionHandler.h"
#include "LiveInterface.h"
#include "PID.h"
#include "WindowManager.h"

// TODO: unsuckify this
// TODO: build out the rest of the map and put it in... somewhere else
std::string keyCodeToString(CGKeyCode keyCode) {
    // Handle special keys
    switch (keyCode) {
        // Function keys
        // NOLINTBEGIN
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
        // NOLINTEND
        
        default:
            UniChar chars[4];
            UniCharCount length = 0;

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

EventHandler::EventHandler(
    std::function<std::shared_ptr<IActionHandler>()> actionHandler
    , std::function<std::shared_ptr<WindowManager>()> windowManager
    )
    : windowManager_(std::move(windowManager))
    , actionHandler_(std::move(actionHandler))
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
            logger->debug("Application is already in focus: " + std::to_string(pid));
            return;
        }

        logger->debug("Bringing app into focus: " + std::to_string(pid));
        [app activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];
    }
}

// TODO TODO Ensure this method is called on the main thread
void EventHandler::focusWindow(void* nativeWindowHandle) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (nativeWindowHandle == nullptr) return;

        [NSApp activateIgnoringOtherApps:YES];

        auto view = (NSView*)nativeWindowHandle;
        if (view != nil) {
            NSWindow* window = [view window];  // Get the NSWindow that contains this NSView
            if (window != nil) {
                [window makeKeyAndOrderFront:nil];  // Bring the window to the front and make it key
                [window makeFirstResponder:view];   // Set the first responder to the view
            }
        }
    });
}

bool EventHandler::isWindowFocused(int windowID) {
    NSArray *windows = [NSApp windows];
    // NOLINTNEXTLINE
    for (NSWindow *window in windows) {
        if ([window windowNumber] == windowID) {
            return [window isKeyWindow];
        }
    }
    return false;
}

void EventHandler::test() {
    // NOLINTNEXTLINE
    for (NSDictionary *windowInfo in getAllWindowsForApp()) {
        logWindowInfo(windowInfo);
    }
}

void EventHandler::logWindowInfo(NSDictionary *windowInfo) {
    // Get window name
    // NOLINTNEXTLINE
    NSString *windowName = (__bridge NSString *)CFDictionaryGetValue((__bridge CFDictionaryRef)windowInfo, kCGWindowName);
    if (!windowName) windowName = @"Unnamed Window";

    // Get window bounds
    // NOLINTNEXTLINE
    NSDictionary *boundsDict = (__bridge NSDictionary *)CFDictionaryGetValue((__bridge CFDictionaryRef)windowInfo, kCGWindowBounds);
    CGRect windowBounds{};
    // NOLINTNEXTLINE
    CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)boundsDict, &windowBounds);

    // Get window ID
    // NOLINTNEXTLINE
    NSNumber *windowID = (__bridge NSNumber *)CFDictionaryGetValue((__bridge CFDictionaryRef)windowInfo, kCGWindowNumber);

    // Get window layer
    // NOLINTNEXTLINE
    NSNumber *windowLayer = (__bridge NSNumber *)CFDictionaryGetValue((__bridge CFDictionaryRef)windowInfo, kCGWindowLayer);

    // Build the debug log string
    std::string debugInfo = "Window ID: " + std::to_string([windowID intValue]) + "\n";
    debugInfo += "Window Name: " + std::string([windowName UTF8String]) + "\n";
    debugInfo += "Window Layer: " + std::to_string([windowLayer intValue]) + "\n";
    debugInfo += "Window Bounds: (" + std::to_string(windowBounds.origin.x) + ", " + std::to_string(windowBounds.origin.y) + 
                 "), Size: (" + std::to_string(windowBounds.size.width) + ", " + std::to_string(windowBounds.size.height) + ")\n";

    // Use logHandler_()->debug to print the debug information
    logger->debug(debugInfo);
}

NSArray *EventHandler::getAllWindowsForApp() {
    NSMutableArray *appWindows = [NSMutableArray array];

    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);

    for (CFIndex i = 0; i < CFArrayGetCount(windowList); i++) {
        // NOLINTNEXTLINE
        CFDictionaryRef windowInfo = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);
        // NOLINTNEXTLINE
        CFNumberRef windowPID = (CFNumberRef)CFDictionaryGetValue(windowInfo, kCGWindowOwnerPID);

        pid_t windowPidValue = -1;
        CFNumberGetValue(windowPID, kCFNumberIntType, &windowPidValue);

        if (windowPidValue == PID::getInstance().livePID()) {
            // Get the window level to determine if it's a floating window or standard window
            // must do this or we'll return the plugin popup's bounds
            // NOLINTNEXTLINE
            CFNumberRef windowLevel = (CFNumberRef)CFDictionaryGetValue(windowInfo, kCGWindowLayer);
            int level = -1;
            CFNumberGetValue(windowLevel, kCFNumberIntType, &level);

            // Live main window is level 0
            // plugin windows are level 3
//            if (level != 0) {
//                continue;  // Skip non-standard windows (likely pop-ups)
//            }
            [appWindows addObject:(__bridge NSDictionary *)windowInfo];

            // NOLINTNEXTLINE
            CFDictionaryRef boundsDict = (CFDictionaryRef)CFDictionaryGetValue(windowInfo, kCGWindowBounds);
            CGRect bounds;
            CGRectMakeWithDictionaryRepresentation(boundsDict, &bounds);
        }
    }

    CFRelease(windowList);

    return appWindows;
}

NSDictionary *getMainWindowForApp(NSArray *appWindows) {
    // Assuming the main window is the first window in the list, or you can use additional criteria
    if ([appWindows count] > 0) {
        return [appWindows firstObject];  // Main window could be the first one, adjust logic as needed
    }
    
    return nil;
}

NSView* EventHandler::getViewFromWindowID(int windowID) {
    NSWindow* window = [NSApp windowWithWindowNumber:windowID];
    if (window) {
        return [window contentView];
    }
    return nil;
}

void* getViewPointerFromWindowID(int windowID) {
    NSWindow* window = [NSApp windowWithWindowNumber:windowID];
    if (window) {
        return (__bridge void*)[window contentView];
    }
    return nil;
}

NSWindow* EventHandler::getWindowFromWindowID(int windowID) {
    NSWindow* window = [NSApp windowWithWindowNumber:windowID];
    if (window) {
        return window;
    }
    return nil;
}

void EventHandler::focusWindow(int windowID) {
    if (windowID == 0) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        logger->debug("focusing window with int windowID");
        void* viewPointer = getViewPointerFromWindowID(windowID);

        if (viewPointer == nullptr) return;

        [NSApp activateIgnoringOtherApps:YES];

        auto* view = (NSView*)viewPointer;
        if (view != nil) {
            NSWindow* window = [view window];  // Get the NSWindow that contains this NSView
            if (window != nil) {
                [window makeKeyAndOrderFront:nil];  // Bring the window to the front and make it key
                [window makeFirstResponder:view];   // Set the first responder to the view
            }
        }
    });
}

//void EventHandler::focusWindow(int windowID) {
//    NSView* view = (NSView*)windowID;
//    if (view != nil) {
//        NSWindow* window = [view window];  // Get the NSWindow that contains this NSView
//        if (window != nil) {
//            [window makeKeyAndOrderFront:nil];  // Bring the window to the front and make it key
//            [window makeFirstResponder:view];   // Set the first responder to the view
//        }
//    }
//}

NSRect getLiveBounds() {
    NSRect appBounds = NSMakeRect(0, 0, 0, 0);

    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
    CFIndex count = CFArrayGetCount(windowList);

    for (CFIndex i = 0; i < count; i++) {
        // NOLINTNEXTLINE
        CFDictionaryRef windowInfo = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

        CFNumberRef windowPID = nullptr;
        // NOLINTNEXTLINE
        windowPID = (CFNumberRef)CFDictionaryGetValue(windowInfo, kCGWindowOwnerPID);

        pid_t windowPidValue = -1;
        CFNumberGetValue(windowPID, kCFNumberIntType, &windowPidValue);

        if (windowPidValue == PID::getInstance().livePID()) {
            // Get the window level to determine if it's a floating window or standard window
            // must do this or we'll return the plugin popup's bounds
            // NOLINTNEXTLINE
            CFNumberRef windowLevel = (CFNumberRef)CFDictionaryGetValue(windowInfo, kCGWindowLayer);
            int level = -1;
            CFNumberGetValue(windowLevel, kCFNumberIntType, &level);

            // Standard windows typically have a layer of 0, floating windows are higher
            if (level != 0) {
                continue;  // Skip non-standard windows (likely pop-ups)
            }

            // NOLINTNEXTLINE
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
    ERect rect{};
    rect.x = static_cast<int>(appBounds.origin.x);
    rect.y = static_cast<int>(appBounds.origin.y);
    rect.width = static_cast<int>(appBounds.size.width);
    rect.height = static_cast<int>(appBounds.size.height);
    return rect;
}

std::optional<std::chrono::steady_clock::time_point> lastRightClickTime;
// TODO: move to config
const int doubleClickThresholdMs = 300;


//TODO 
// Handling Mouse Clicks: Instead of logging or handling double-click events
// directly in the UI, you can use the AX API to check if certain elements
// are under the mouse cursor and interact with them accordingly.
// 
// Handling Key Presses: When detecting key presses, instead of processing
// them for an AppKit UI element, you might want to query if certain
// accessibility elements are focused or in edit mode, and then modify their
// content (e.g., typing into a text field).

// TODO
//std::shared_ptr<LiveInterface> liveInterface = std::make_shared<LiveInterface>();

CGEventRef EventHandler::eventTapCallback(CGEventTapProxy proxy, CGEventType ogEventType, CGEventRef ogEvent, void *refcon) {
    // fix some edge cases where event gets destroyed
    // NOLINTNEXTLINE
    CGEventRef event = (CGEventRef)CFRetain(ogEvent);
    CGEventType eventType = CGEventGetType(event);
    auto* handler = static_cast<EventHandler*>(refcon);

    // TODO store/pass these so we don't have to call them every time
    // or at least only call these where necessary
    // and/or call the log via the singleton interface
    auto windowManager = handler->windowManager_();
    auto actionHandler = handler->actionHandler_();

    // 40 is weird. The normal flag didn't work
    const auto magicFieldThatActuallyGetsPID = 40;
    auto eventPID = (pid_t)CGEventGetIntegerValueField(event, (CGEventField)magicFieldThatActuallyGetsPID);

    // close the search box when clicking in Live
    if (windowManager->isWindowOpen("SearchBox")) {
        if (eventType == kCGEventLeftMouseDown || eventType == kCGEventRightMouseDown || eventType == kCGEventOtherMouseDown) {
            dispatch_async(dispatch_get_main_queue(), ^{
                //handler->log_->info("event pid: " + std::to_string(eventPID));
                //handler->log_->info("app pid: " + std::to_string(PID::getInstance().appPID()));
                //handler->log_->info("live pid: " + std::to_string(PID::getInstance().livePID()));
                
                auto targetPID = (pid_t)CGEventGetIntegerValueField(event, kCGEventTargetUnixProcessID);
                CGPoint location = CGEventGetLocation(event);
                CFRelease(event);

                logger->info("target pid: " + std::to_string(targetPID));

                // NOLINTNEXTLINE
                auto* nativeView = reinterpret_cast<NSView *>(windowManager->getWindowHandle("SearchBox"));
                logger->debug("got window handle");

                NSWindow *searchBoxWindow = [nativeView window];

                NSRect appBounds = [searchBoxWindow frame];

                CGPoint mouseLocation = [NSEvent mouseLocation];
                bool isOutsideApp = !NSPointInRect(mouseLocation, appBounds);

                logger->debug("Mouse Location: " + std::to_string(mouseLocation.x) + ", " + std::to_string(mouseLocation.y));
                logger->debug("App Bounds: " + std::to_string(appBounds.origin.x) + ", " + std::to_string(appBounds.origin.y) + " to " + std::to_string(appBounds.origin.x + appBounds.size.width) + ", " + std::to_string(appBounds.origin.y + appBounds.size.height));

                if (isOutsideApp) {
                    NSRect liveBounds = getLiveBounds();
                    bool isInsideLive = NSPointInRect(mouseLocation, liveBounds);
                    logger->debug("Live Bounds: " + std::to_string(liveBounds.origin.x) + ", " + std::to_string(liveBounds.origin.y) + " to " + std::to_string(liveBounds.origin.x + liveBounds.size.width) + ", " + std::to_string(liveBounds.origin.y + liveBounds.size.height));

                    if (isInsideLive) {
                        logger->debug("Click is outside app, inside Live, closing window.");
                        windowManager->closeWindow("SearchBox");
    //                    return NULL;
                    }
                } else {
                    logger->debug("Click is inside the window, keeping window open.");
                }
            });
        }
    }

    if (eventPID == PID::getInstance().livePID()) {
        // double-right-click menu
        if (eventType == kCGEventRightMouseDown) {
            logger->debug("Ableton Live right click event detected.");
            //handler->log_->info("right click event");
            auto now = std::chrono::steady_clock::now();

            if (lastRightClickTime.has_value()) {
                auto durationSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRightClickTime.value());
                logger->debug("Duration since last click: " + std::to_string(durationSinceLastClick.count()) + " ms");

                if (durationSinceLastClick.count() != 0 && durationSinceLastClick.count() <= doubleClickThresholdMs) {
                    //handler->log_->info(        logger->debug("Termination detected for PID: " + std::to_string(terminatedPID));"double right click");
                    dispatch_async(dispatch_get_main_queue(), ^{
                        actionHandler->handleDoubleRightClick();
                    });
                    return nullptr;
                }
            } else {
                // handler->log_->info("First right click detected, skipping double-click check");
            }

            lastRightClickTime = now;
            return event;

        } else if (eventType == kCGEventKeyDown) {
            logger->debug("Ableton Live keydown event detected.");

// TODO fix
//            if (liveInterface->isAnyTextFieldFocused()) {
//                logger->debug("Ableton Live text field has focus. Passing event.");
//                return event;
//            }

            // TODO
            // You can use the CGWindowListCopyWindowInfo function to get information about
            // all windows on the screen and correlate it with the window receiving the event
            // like see if the event app bounds match live's to determine if live is receiving
            // the event vs a plugin window

            CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            CGEventFlags flags = CGEventGetFlags(event);

            EKeyPress pressedKey;
            pressedKey.state = KeyState::Down;
            pressedKey.shift = (flags & Shift) != 0;
            pressedKey.ctrl  = (flags & Ctrl ) != 0;
            pressedKey.cmd   = (flags & Cmd  ) != 0;
            pressedKey.alt   = (flags & Alt  ) != 0;
            pressedKey.key   = keyCodeToString(keyCode);

            bool shouldPassEvent = actionHandler->handleKeyEvent(pressedKey);

            return shouldPassEvent ? event : nullptr;
        }
    }

    return event;
}

void EventHandler::setupQuartzEventTap() {
    auto log = logger;
    logger->debug("EventHandler::setupQuartEventTap() called");

    if (PID::getInstance().livePID() == -1) {
        logger->error("EventHandler: Ableton Live not found.");
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
        logger->error("EventHandler: Failed to create event tap.");
        return;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    logger->debug("EventHandler: Quartz event tap is active!");
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

void EventHandler::registerAppLaunch(std::function<void()> onLaunchCallback) {
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserverForName:NSWorkspaceDidLaunchApplicationNotification
                                                                    object:nil
                                                                     queue:nil
                                                                usingBlock:^(NSNotification *notification) {
        NSDictionary *userInfo = [notification userInfo];
        NSRunningApplication *launchedApp = [userInfo objectForKey:NSWorkspaceApplicationKey];

        pid_t launchedPID = [launchedApp processIdentifier];
        NSString *bundleID = [launchedApp bundleIdentifier];

        if ([bundleID isEqualToString:@"com.ableton.live"]) {
            logger->info("Target app launched with PID: " + std::to_string(launchedPID));
            if (onLaunchCallback) {
                onLaunchCallback();
            }
        }
    }];
}

void EventHandler::registerAppTermination(std::function<void()> onTerminationCallback) {
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserverForName:NSWorkspaceDidTerminateApplicationNotification
                                                                    object:nil
                                                                     queue:nil
                                                                usingBlock:^(NSNotification *notification) {
        NSDictionary *userInfo = [notification userInfo];
        NSRunningApplication *terminatedApp = [userInfo objectForKey:NSWorkspaceApplicationKey];
        NSString *terminatedBundleID = [terminatedApp bundleIdentifier];

        logger->debug("Termination detected for bundle ID: " + std::string([terminatedBundleID UTF8String]));

        NSString *targetBundleID = @"com.ableton.live";
        if ([terminatedBundleID isEqualToString:targetBundleID]) {
            logger->info("Target application with bundle ID " + std::string([terminatedBundleID UTF8String]) + " has been terminated");
            if (onTerminationCallback) {
                onTerminationCallback();
            }
        }
    }];
}
