#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <ApplicationServices/ApplicationServices.h>
#include <string>

class EventHandler {
public:
    EventHandler();
    ~EventHandler();

    void setupQuartzEventTap();
    void runPlatform();

private:
    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
};

#endif
