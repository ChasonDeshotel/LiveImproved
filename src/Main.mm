#include <ApplicationServices/ApplicationServices.h>
#include <QApplication>

#include "LogHandler.h"

#include "ApplicationManager.h"
#include "PlatformDependent.h"
#include "ActionHandler.h"

#ifdef INJECTED_LIBRARY

__attribute__((constructor))
static void dylib_init() {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        LogHandler::getInstance().info("injected successfully");
        ApplicationManager& app = ApplicationManager::getInstance();

        EventHandler eventHandler(app);
        ActionHandler actionHandler(app);
        KeySender keySender(app);

        app.init();

        // Initialize ApplicationManager with these modules
        ApplicationManager::getInstance();
    });
}

#else

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Create a hidden dummy window
    NSWindow *dummyWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1, 1)
                                                        styleMask:NSWindowStyleMaskBorderless
                                                          backing:NSBackingStoreBuffered
                                                            defer:NO];
    [dummyWindow setIsVisible:NO];  // Hide the window
    [NSApp setWindowsMenu:nil];     // Optional: Removes the default Windows menu
}

@end

int main(int argc, char *argv[]) {
    LogHandler::getInstance().info("Application started");

    static QApplication app(argc, argv);

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

        // Custom initialization logic
        ApplicationManager& app = ApplicationManager::getInstance();
        ActionHandler actionHandler(app);
        KeySender keySender(app);
        app.init();

        // Setup event tap for Quartz events
        EventHandler handler(app);
        handler.setupQuartzEventTap();

        LogHandler::getInstance().info("Initialization complete");

        // Start the NSApplication run loop
        // IF INJECTED_LIBRARY
        //CFRunLoopRun();
        // ELSE
        // Start run loop
        [NSApp run];
    }

    [NSApp finishLaunching];

    return app.exec();

    //LogHandler::getInstance().info("Application exiting");

    //return 0;
}

#endif
