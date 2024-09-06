#include "PlatformInitializer.h"
#include "LogHandler.h"
#include <Cocoa/Cocoa.h>

void PlatformInitializer::init() {
    LogHandler::getInstance().info("Platform initialization started");

    @autoreleasepool {
        // Initialize Cocoa Application
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

        // Dummy window creation to prevent macOS from automatically creating a new window
        NSWindow *dummyWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1, 1)
                                                            styleMask:NSWindowStyleMaskBorderless
                                                              backing:NSBackingStoreBuffered
                                                                defer:NO];
        [dummyWindow setIsVisible:NO];

        LogHandler::getInstance().info("Cocoa application initialized");

    }
}

void PlatformInitializer::run() {
    LogHandler::getInstance().info("Finishing Cocoa application");

    @autoreleasepool {
        // Start the NSApplication run loop
        [NSApp run];

        [NSApp finishLaunching];
    }

    LogHandler::getInstance().info("Cocoa application finished launching");
}
