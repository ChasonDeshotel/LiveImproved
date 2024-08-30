#include <ApplicationServices/ApplicationServices.h>

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

int main(int argc, const char * argv[]) {
    LogHandler::getInstance().info("Application started");

    @autoreleasepool {
        [NSApplication sharedApplication];

        ApplicationManager& app = ApplicationManager::getInstance();

        ActionHandler actionHandler(app);
        KeySender keySender(app);

        app.init();

        // Setup application
        //NSApplication *app = [NSApplication sharedApplication];
        //[app activateIgnoringOtherApps:YES];
        
        // Setup event tap
        EventHandler handler(ApplicationManager::getInstance());
        handler.setupQuartzEventTap();

        LogHandler::getInstance().info("Initialization complete");

        // IF INJECTED_LIBRARY
        //CFRunLoopRun();
        // ELSE
        // Start run loop
        [NSApp run];
    }

    LogHandler::getInstance().info("Application exiting");

    return 0;
}

#endif
