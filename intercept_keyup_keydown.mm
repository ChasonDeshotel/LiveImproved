#include <iostream>
#include <fstream>
#include <objc/runtime.h>
#include <Cocoa/Cocoa.h>

// Define the function pointer types for the original methods
typedef void (*OriginalKeyDownType)(id, SEL, NSEvent*);
typedef void (*OriginalKeyUpType)(id, SEL, NSEvent*);

// Original method pointers
static OriginalKeyDownType originalKeyDown = nullptr;
static OriginalKeyUpType originalKeyUp = nullptr;

// Path to the log file
static const char *logFilePath = "/Users/cdeshotel/Scripts/Ableton/InterceptKeys/log.txt";

// Utility function to log messages to a file
void logToFile(const std::string &message) {
    std::ofstream logfile;
    logfile.open(logFilePath, std::ios_base::app);
    if (logfile.is_open()) {
        logfile << message << std::endl;
        logfile.close();
    }
}

// Swizzled keyDown: method
void swizzledKeyDown(id self, SEL _cmd, NSEvent *event) {
    logToFile("swizzledKeyDown: method called");
    logToFile("Key code: " + std::to_string([event keyCode]));

    // Example: Handle specific keys differently
    if ([event keyCode] == 40) {  // 'K' key
        logToFile("K key pressed! Triggering custom action.");
        // Custom logic for 'K' key
    } else if ([event modifierFlags] & NSEventModifierFlagCommand) {
        logToFile("Command key with another key pressed.");
        // Custom logic for Command + key combinations
    }

    // Call the original keyDown: method to ensure normal behavior continues
    if (originalKeyDown) {
        originalKeyDown(self, _cmd, event);
    }
}

// Swizzled keyUp: method
void swizzledKeyUp(id self, SEL _cmd, NSEvent *event) {
    logToFile("swizzledKeyUp: method called");
    logToFile("Key code: " + std::to_string([event keyCode]));

    // Example: Handle key release
    if ([event keyCode] == 40) {  // 'K' key
        logToFile("K key released!");
        // Custom logic for 'K' key release
    }

    // Call the original keyUp: method
    if (originalKeyUp) {
        originalKeyUp(self, _cmd, event);
    }
}

// Function to swizzle keyDown: and keyUp: methods
__attribute__((constructor))
void swizzleKeyEvents() {
    logToFile("Attempting to swizzle keyDown: and keyUp: methods in NSResponder");

    Class nsWindowClass = objc_getClass("NSResponder");

    if (!nsWindowClass) {
        logToFile("Failed to get NSResponder class!");
        return;
    }

    // Swizzle keyDown:
    SEL keyDownSelector = @selector(keyDown:);
    Method keyDownMethod = class_getInstanceMethod(nsWindowClass, keyDownSelector);
    if (keyDownMethod) {
        // Perform method swizzling
        IMP originalIMP = method_getImplementation(keyDownMethod);
        originalKeyDown = (OriginalKeyDownType)originalIMP;

        if (originalIMP) {
            method_setImplementation(keyDownMethod, (IMP)swizzledKeyDown);
            logToFile("Swizzle for keyDown: in NSResponder is active!");
        } else {
            logToFile("Failed to get the original keyDown: implementation!");
        }
    } else {
        logToFile("Failed to find keyDown: method in NSResponder!");
    }

    // Swizzle keyUp:
    SEL keyUpSelector = @selector(keyUp:);
    Method keyUpMethod = class_getInstanceMethod(nsWindowClass, keyUpSelector);
    if (keyUpMethod) {
        // Perform method swizzling
        IMP originalIMP = method_getImplementation(keyUpMethod);
        originalKeyUp = (OriginalKeyUpType)originalIMP;

        if (originalIMP) {
            method_setImplementation(keyUpMethod, (IMP)swizzledKeyUp);
            logToFile("Swizzle for keyUp: in NSResponder is active!");
        } else {
            logToFile("Failed to get the original keyUp: implementation!");
        }
    } else {
        logToFile("Failed to find keyUp: method in NSResponder!");
    }
}
