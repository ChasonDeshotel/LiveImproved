#include <ApplicationServices/ApplicationServices.h>

#include "lib/LogHandler.h"

#include "lib/ApplicationManager.h"
#include "lib/PlatformDependent.h"
#include "lib/ActionHandler.h"

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
