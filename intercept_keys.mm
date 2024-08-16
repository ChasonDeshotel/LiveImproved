#include <iostream>
#include <fstream>
#include <objc/runtime.h>

#import "intercept_keys.h"

#include "menu.h"

OriginalHandleHotKeyPressedType originalHandleHotKeyPressed = nullptr;
OriginalHandleHotKeyReleaseType originalHandleHotKeyRelease = nullptr;

__attribute__((constructor))
void initialize() {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(6 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        setupQuartzEventTap();
    });

		setAbletonLivePID();
}

void logToFile(const std::string &message) {
    std::ofstream logfile;
    logfile.open(logFilePath, std::ios_base::app);
    if (logfile.is_open()) {
        logfile << message << std::endl;
        logfile.close();
    }
}

pid_t getPID(NSString *processName) {
    NSArray *runningApps = [[NSWorkspace sharedWorkspace] runningApplications];
    
    for (NSRunningApplication *app in runningApps) {
        NSString *executablePath = [[app executableURL] path];
        if ([executablePath containsString:processName]) {
            return [app processIdentifier];
        }
    }
    
    // If the app is not found, return -1
    return -1;
}

void introspect() {
	unsigned int methodCount = 0;
	Method *methods = class_copyMethodList(objc_getClass("NSApplication"), &methodCount);

	for (unsigned int i = 0; i < methodCount; i++) {
			SEL methodName = method_getName(methods[i]);
			const char *name = sel_getName(methodName);
			logToFile("Method name: " + std::string(name));
	}

	free(methods);
}

// was the event custom handled
BOOL swizzledPerformKeyEquivalent(id self, SEL _cmd, NSEvent *event) {
    logToFile("swizzledPerformKeyEquivalent: method called");
    logToFile("Key code: " + std::to_string([event keyCode]));

//    if ([event keyCode] == 40 && ([event modifierFlags] & NSEventModifierFlagCommand)) {
//        logToFile("Command + K pressed! Triggering custom action.");
//				introspect();
        // Custom logic here
//        return YES; // Indicate that the event was handled
//    }

    if (originalPerformKeyEquivalent) {
        return originalPerformKeyEquivalent(self, _cmd, event);
    } else {
        return NO;
    }
}

__attribute__((constructor))
void swizzlePerformKeyEquivalent() {
    logToFile("Attempting to swizzle performKeyEquivalent: method in NSWindow");

    // Update the class to NSWindow or NSResponder
    //Class nsWindowClass = objc_getClass("NSWindow");
    Class nsWindowClass = objc_getClass("NSView");

    if (!nsWindowClass) {
        logToFile("Failed to get NSWindow class!");
        return;
    }

    SEL performKeyEquivalentSelector = @selector(performKeyEquivalent:);
    Method performKeyEquivalentMethod = class_getInstanceMethod(nsWindowClass, performKeyEquivalentSelector);

    if (performKeyEquivalentMethod) {
        // Perform method swizzling
        IMP originalIMP = method_getImplementation(performKeyEquivalentMethod);
        originalPerformKeyEquivalent = (OriginalPerformKeyEquivalentType)originalIMP;

        if (originalIMP) {
            method_setImplementation(performKeyEquivalentMethod, (IMP)swizzledPerformKeyEquivalent);
            logToFile("Swizzle for performKeyEquivalent: in NSWindow is active!");
        } else {
            logToFile("Failed to get the original performKeyEquivalent: implementation!");
        }
    } else {
        logToFile("Failed to find performKeyEquivalent: method in NSWindow!");
    }
}

//__attribute__((destructor))
//void unswizzlePerformKeyEquivalent() {
//    if (originalPerformKeyEquivalent) {
//        Class nsWindowClass = objc_getClass("NSWindow");
//        SEL performKeyEquivalentSelector = @selector(performKeyEquivalent:);
//        Method performKeyEquivalentMethod = class_getInstanceMethod(nsWindowClass, performKeyEquivalentSelector);
//        if (performKeyEquivalentMethod) {
//            method_setImplementation(performKeyEquivalentMethod, (IMP)originalPerformKeyEquivalent);
//            logToFile("Unswizzled performKeyEquivalent: method in NSWindow, original method restored");
//        } else {
//            logToFile("Failed to find performKeyEquivalent: method in NSWindow for unswizzling!");
//        }
//    } else {
//        logToFile("Original performKeyEquivalent: method in NSWindow was null, nothing to unswizzle");
//    }
//}

// Swizzled method for _handleHotKeyPressed:
void swizzledHandleHotKeyPressed(id self, SEL _cmd, NSEvent *event) {
    logToFile("_handleHotKeyPressed: method called");
    logToFile("Key code: " + std::to_string([event keyCode]) + ", Modifiers: " + std::to_string([event modifierFlags]));

    // Custom logic for handling the hotkey event
    if ([event keyCode] == 40 && ([event modifierFlags] & NSEventModifierFlagCommand)) {
        logToFile("Command + K hotkey pressed! Triggering custom action.");
    }

    // Call the original implementation
    if (originalHandleHotKeyPressed) {
        originalHandleHotKeyPressed(self, _cmd, event);
    }
}

// Swizzled method for _handleHotKeyRelease:
void swizzledHandleHotKeyRelease(id self, SEL _cmd, NSEvent *event) {
    logToFile("_handleHotKeyRelease: method called");
    logToFile("Key code: " + std::to_string([event keyCode]) + ", Modifiers: " + std::to_string([event modifierFlags]));

    // Call the original implementation
    if (originalHandleHotKeyRelease) {
        originalHandleHotKeyRelease(self, _cmd, event);
    }
}

// Function to swizzle _handleHotKeyPressed:
__attribute__((constructor))
void swizzleHandleHotKeyPressed() {
    Class nsAppClass = objc_getClass("NSApplication");

    SEL selector = NSSelectorFromString(@"_handleHotKeyPressed:");
    Method method = class_getInstanceMethod(nsAppClass, selector);

    if (method) {
        originalHandleHotKeyPressed = (OriginalHandleHotKeyPressedType)method_getImplementation(method);
        method_setImplementation(method, (IMP)swizzledHandleHotKeyPressed);
        logToFile("Swizzle for _handleHotKeyPressed: in NSApplication is active!");
    } else {
        logToFile("Failed to find _handleHotKeyPressed: method in NSApplication!");
    }
}

// Function to swizzle _handleHotKeyRelease:
__attribute__((constructor))
void swizzleHandleHotKeyRelease() {
    Class nsAppClass = objc_getClass("NSApplication");

    SEL selector = NSSelectorFromString(@"_handleHotKeyRelease:");
    Method method = class_getInstanceMethod(nsAppClass, selector);

    if (method) {
        originalHandleHotKeyRelease = (OriginalHandleHotKeyReleaseType)method_getImplementation(method);
        method_setImplementation(method, (IMP)swizzledHandleHotKeyRelease);
        logToFile("Swizzle for _handleHotKeyRelease: in NSApplication is active!");
    } else {
        logToFile("Failed to find _handleHotKeyRelease: method in NSApplication!");
    }
}

void setAbletonLivePID() {
    NSString *appName = @"Ableton Live 11 Suite";
    pid_t pidFromApp = getPID(appName);

    if (pidFromApp <= 0) {
        logToFile("Failed to get Ableton Live PID");
        return;
    }

    abletonLivePID = pidFromApp;
    logToFile("Ableton Live found with PID: " + std::to_string(abletonLivePID));

    // Continue with your existing logic to match the PID in the event tap...
}

CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    // Get the PID of the event's source process
		pid_t eventPID = (pid_t)CGEventGetIntegerValueField(event, 40); 

    // Only handle events from Ableton Live's PID
    if (eventPID == abletonLivePID) {
        logToFile("Ableton Live event detected.");
        // Process the event
				//for (int i = 0; i < kCGEventSourceStateID + 10; i++) {
				//		int64_t value = CGEventGetIntegerValueField(event, i);
				//		logToFile("Field " + std::to_string(i) + ": " + std::to_string(value));
				//}
				//logToFile("--------");

				CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
				CGEventFlags flags = CGEventGetFlags(event);
				logToFile("Key event: " + std::string(type == kCGEventKeyDown ? "KeyDown" : "KeyUp") + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

        //if (type == kCGEventKeyDown && keyCode == 37 && (CGEventGetFlags(event) & kCGEventFlagMaskCommand)) {
        if (type == kCGEventKeyDown && keyCode == 18 && (CGEventGetFlags(event))) {

						// Log key events
            logToFile("sending new key event");

            CGEventRef newKeyDownEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)46, true); // 37 is the keyCode for 'L'
						CGEventFlags flags = kCGEventFlagMaskCommand | kCGEventFlagMaskShift;
            CGEventSetFlags(newKeyDownEvent, flags);
            CGEventPost(kCGAnnotatedSessionEventTap, newKeyDownEvent);

            CGEventRef newKeyUpEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)46, false); // Key up event
						CGEventSetFlags(newKeyDownEvent, flags);
            CGEventPost(kCGAnnotatedSessionEventTap, newKeyUpEvent);

            CFRelease(newKeyDownEvent);
            CFRelease(newKeyUpEvent);

            return NULL; // Block the original event
        }
		}

		return event;
}

// Function to set up the Quartz event tap
//void setupQuartzEventTap() {
//
//		logToFile("setup quartz event tap");
//    // Create an event tap to capture key events
//    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
//   // CFMachPortRef eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, eventMask, eventTapCallback, NULL);
//		CFMachPortRef eventTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, 0, eventMask, eventTapCallback, NULL);
//
//    if (!eventTap) {
//        logToFile("Failed to create event tap.");
//        return;
//    }
//
//    // Create a run loop source and add it to the current run loop
//    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
//    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
//    CGEventTapEnable(eventTap, true);
//    logToFile("Quartz event tap is active!");
//
//		logToFile("before run loop");
//    // Start the run loop
//    CFRunLoopRun();
//		logToFile("after run loop");
//}

#include <ApplicationServices/ApplicationServices.h>
#include <unistd.h> // For getpid()

void setupQuartzEventTap() {
    setAbletonLivePID();  // Set the PID of Ableton Live

    if (abletonLivePID == 0) {
        logToFile("Ableton Live not found.");
        return;
    }

    CGEventMask eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
    CFMachPortRef eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, eventMask, eventTapCallback, NULL);

    if (!eventTap) {
        logToFile("Failed to create event tap.");
        return;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    logToFile("Quartz event tap is active!");

    CFRunLoopRun();
}
