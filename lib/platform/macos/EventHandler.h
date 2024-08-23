#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <ApplicationServices/ApplicationServices.h>

class ApplicationManager;

class EventHandler {
public:
    EventHandler(ApplicationManager& appManager);
    ~EventHandler();

    void initialize();

    void setAbletonLivePID();

    void setupQuartzEventTap();
    void runPlatform();

private:
    ApplicationManager& app_;

    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
};

#endif
