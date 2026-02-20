#include <Cocoa/Cocoa.h>

#include "LogGlobal.h"

#include "PlatformInitializer.h"

bool PlatformInitializer::checkPrivileges() {
    if (!AXIsProcessTrusted()) {
        NSDictionary* options = @{(__bridge NSString*)kAXTrustedCheckOptionPrompt: @YES};
        if (!AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options)) {
            // TODO idk something smartly that doesn't require reloading the app
            return false;
        }
    }
    //NSDictionary* options = @{(__bridge NSString*)kAXTrustedCheckOptionPrompt: @YES};
    //AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
    //    NSString* prefPage = @"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility";
    //    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:prefPage]];
    //    sleep(15);
    //    return checkPrivileges();
    //}
    return true;
}

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
}

void PlatformInitializer::stop() {
    @autoreleasepool {
        [NSApp stop:nil];
        // post a dummy event to wake the run loop so stop takes effect
        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                           location:NSMakePoint(0, 0)
                                      modifierFlags:0
                                          timestamp:0
                                       windowNumber:0
                                            context:nil
                                            subtype:0
                                              data1:0
                                              data2:0];
        [NSApp postEvent:event atStart:YES];
    }
}