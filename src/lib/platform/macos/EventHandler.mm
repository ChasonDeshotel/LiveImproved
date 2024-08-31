#include <ApplicationServices/ApplicationServices.h>
#include <string>
#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>

#include "ApplicationManager.h"
#include "EventHandler.h"
#include "LogHandler.h"
#include "PID.h"

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

NSRect getAppBoundsByPID(pid_t pid) {
    NSRect appBounds = NSMakeRect(0, 0, 0, 0);

    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
    CFIndex count = CFArrayGetCount(windowList);

    for (CFIndex i = 0; i < count; i++) {
        CFDictionaryRef windowInfo = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

        CFNumberRef windowPID;
        windowPID = (CFNumberRef)CFDictionaryGetValue(windowInfo, kCGWindowOwnerPID);

        pid_t windowPidValue;
        CFNumberGetValue(windowPID, kCFNumberIntType, &windowPidValue);

        if (windowPidValue == pid) {
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

CGEventRef EventHandler::eventTapCallback(CGEventTapProxy proxy, CGEventType eventType, CGEventRef event, void *refcon) {
    EventHandler* handler = static_cast<EventHandler*>(refcon);

		pid_t eventPID = (pid_t)CGEventGetIntegerValueField(event, (CGEventField)40);
    CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    CGEventFlags flags = CGEventGetFlags(event);


    if (handler->app_.getGUISearchBox()->isOpen()) {
        if (eventType == kCGEventLeftMouseDown || eventType == kCGEventRightMouseDown || eventType == kCGEventOtherMouseDown) {
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
                NSRect liveBounds = getAppBoundsByPID(PID::getInstance().livePID());
                bool isInsideLive = NSPointInRect(mouseLocation, liveBounds);
                LogHandler::getInstance().info("Live Bounds: " + std::to_string(liveBounds.origin.x) + ", " + std::to_string(liveBounds.origin.y) + " to " + std::to_string(liveBounds.origin.x + liveBounds.size.width) + ", " + std::to_string(liveBounds.origin.y + liveBounds.size.height));

                if (isInsideLive) {
                    LogHandler::getInstance().info("Click is outside app, inside Live, closing window.");
                    handler->app_.getGUISearchBox()->closeSearchBox();
                    return NULL;
                }
            } else {
                LogHandler::getInstance().info("Click is inside the window, keeping window open.");
            }

        }
//            // Get the PID of the process that received the event
//            
//            // Define the PID of the application you're interested in
//            pid_t specificAppPID = 12345; // Replace with the actual PID
//
//            // Check if the event was targeted at the specific application
//            if (targetPID == specificAppPID) {
//                // Perform your logic here, e.g., log, take action, etc.
//                printf("User clicked into the application with PID %d\n", specificAppPID);
//            }
//        }
    }

    // TODO: appPID i think needs to be livePID when
    // injected library
    //
    // need to capture clicks as well
//    if (handler->app_.getGUISearchBox()->isOpen()
//        && (eventPID == PID::getInstance().appPID() 
//        || eventPID == PID::getInstance().livePID())
//        )
//    {
//        handler->log_->info("event pid: " + std::to_string(eventPID));
//        handler->log_->info("app pid: " + std::to_string(PID::getInstance().appPID()));
//        if (eventPID == PID::getInstance().livePID()) {
//            EventHandler::focusApplication(PID::getInstance().appPID());
//        }
//
//        // Forward the event to the search box window
//        NSEvent *event = [NSEvent keyEventWithType:NSEventTypeKeyDown
//                                           location:NSMakePoint(0, 0)
//                                      modifierFlags:flags
//                                          timestamp:[[NSProcessInfo processInfo] systemUptime]
//                                       windowNumber:[[NSApp mainWindow] windowNumber]
//                                            context:nil
//                                         characters:@""
//                        charactersIgnoringModifiers:@""
//                                          isARepeat:NO
//                                            keyCode:keyCode];
//
////        GUISearchBoxWindowController* controller = (GUISearchBoxWindowController*)handler->app_.getGUISearchBox()->getWindowController();
////        [controller.window sendEvent:event];
//
//        return NULL;
//
//    } else if (eventPID == PID::getInstance().livePID()) {
    if (eventPID == PID::getInstance().livePID()) {
        handler->log_->info("Ableton Live event detected.");

    //				CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    //				CGEventFlags flags = CGEventGetFlags(event);

        int flagsInt = (int)CGEventGetFlags(event);
        std::string keyUpDown = (eventType == kCGEventKeyDown) ? "keyDown" : "keyUp";


        bool shouldPassEvent = handler->app_.getActionHandler()->handleKeyEvent(static_cast<int>(keyCode), flagsInt, keyUpDown);

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

    CGEventMask eventMask = (1 << kCGEventKeyDown) | 
                           (1 << kCGEventKeyUp) | 
                           (1 << kCGEventLeftMouseDown) | 
                           (1 << kCGEventRightMouseDown) | 
                           (1 << kCGEventOtherMouseDown);
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

