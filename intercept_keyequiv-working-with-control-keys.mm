#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>


typedef BOOL (*OriginalPerformKeyEquivalentType)(id, SEL, NSEvent*);
static OriginalPerformKeyEquivalentType originalPerformKeyEquivalent = nullptr;

static const char *logFilePath = "/Users/cdeshotel/Scripts/Ableton/InterceptKeys/log.txt";
void logToFile(const std::string &message) {
    std::ofstream logfile;
    logfile.open(logFilePath, std::ios_base::app);
    if (logfile.is_open()) {
        logfile << message << std::endl;
        logfile.close();
    }
}

BOOL swizzledPerformKeyEquivalent(id self, SEL _cmd, NSEvent *event) {
    logToFile("swizzledPerformKeyEquivalent: method called");
    logToFile("Key code: " + std::to_string([event keyCode]));

    if ([event keyCode] == 40 && ([event modifierFlags] & NSEventModifierFlagCommand)) {
        logToFile("Command + K pressed! Triggering custom action.");
        // Custom logic here
        return YES; // Indicate that the event was handled
    }

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

__attribute__((destructor))
void unswizzlePerformKeyEquivalent() {
    if (originalPerformKeyEquivalent) {
        Class nsWindowClass = objc_getClass("NSWindow");
        SEL performKeyEquivalentSelector = @selector(performKeyEquivalent:);
        Method performKeyEquivalentMethod = class_getInstanceMethod(nsWindowClass, performKeyEquivalentSelector);
        if (performKeyEquivalentMethod) {
            method_setImplementation(performKeyEquivalentMethod, (IMP)originalPerformKeyEquivalent);
            logToFile("Unswizzled performKeyEquivalent: method in NSWindow, original method restored");
        } else {
            logToFile("Failed to find performKeyEquivalent: method in NSWindow for unswizzling!");
        }
    } else {
        logToFile("Original performKeyEquivalent: method in NSWindow was null, nothing to unswizzle");
    }
}
