#include <Cocoa/Cocoa.h>

#include "LogGlobal.h"

#include "PlatformInitializer.h"

void PlatformInitializer::init() {
    logger->info("Platform initialization started");

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

        logger->debug("Cocoa application initialized");

    }
}

void PlatformInitializer::run() {
    logger->debug("Finishing Cocoa application");

    @autoreleasepool {
        // Start the NSApplication run loop
        [NSApp run];

        [NSApp finishLaunching];
    }

    logger->info("Platform initialization complete");
}
