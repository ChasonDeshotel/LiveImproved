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

        PID pid(app);
        EventHandler eventHandler(app);
        ActionHandler actionHandler(app);
        KeySender keySender(app);

        app.init();

        // Initialize ApplicationManager with these modules
        ApplicationManager::getInstance();
    });
}

#else

int main(int argc, const char * argv[]) {
    LogHandler::getInstance().info("Application started");

    ApplicationManager& app = ApplicationManager::getInstance();

    PID pid(app);
    EventHandler eventHandler(app);
    ActionHandler actionHandler(app);
    KeySender keySender(app);

    app.init();

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        LogHandler::getInstance().info("Initialization complete");

    });

    CFRunLoopRun();

    LogHandler::getInstance().info("Application exiting");

    return 0;
}

#endif
