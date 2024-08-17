#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <ApplicationServices/ApplicationServices.h>
#include <unistd.h> // For getpid()
#include <Cocoa/Cocoa.h>

#import "Main.h"

#include "CustomAlert.h"

CustomAlert *customAlert = nullptr;

void showCustomAlert() {
    customAlert = [[CustomAlert alloc] initWithTitle:@"Custom Searchable Menu"];
    
    if (customAlert) {
        [customAlert showWindow:nil];
        [[customAlert window] center];
        [[customAlert window] makeKeyAndOrderFront:nil];

        // Run the window modally if needed
        [NSApp runModalForWindow:[customAlert window]];
    }
}

__attribute__((constructor))
void initialize() {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
				setAbletonLivePID();
        setupQuartzEventTap();
    });
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

				//if (customAlert && customAlert.isOpen) {
				//		// Log that the event is going to the custom alert window
				//		logToFile("Event sent to custom alert window.");
				//		NSEvent* nsEvent = [NSEvent eventWithCGEvent:event];
				//		[NSApp sendEvent:nsEvent];
				//		return NULL;
				//}
        // Process the event
				//for (int i = 0; i < kCGEventSourceStateID + 10; i++) {
				//		int64_t value = CGEventGetIntegerValueField(event, i);
				//		logToFile("Field " + std::to_string(i) + ": " + std::to_string(value));
				//}
				//logToFile("--------");

				CGKeyCode keyCode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
				CGEventFlags flags = CGEventGetFlags(event);
				logToFile("Key event: " + std::string(type == kCGEventKeyDown ? "KeyDown" : "KeyUp") + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

        if (type == kCGEventKeyDown && keyCode == 19 && (CGEventGetFlags(event))) {
					if (!customAlert || (customAlert && !customAlert.isOpen)) {
						logToFile("showing custom alert");
						showCustomAlert();
					}
				}

        if (keyCode == 53 && type == kCGEventKeyDown && customAlert.searchText.length == 0) { // Escape key
            logToFile("closing menu.");
            if (customAlert && customAlert.isOpen) {
								[customAlert closeAlert];
                customAlert = nullptr;
								return NULL;
						}
        }

				// when the menu is open, do not send keypresses to Live
				if (customAlert && customAlert.isOpen) {
						return event;
				}

				// 1 sends CMD+M
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
