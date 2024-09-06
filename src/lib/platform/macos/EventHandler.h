#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <ApplicationServices/ApplicationServices.h>

class ApplicationManager;
class LogHandler;
class ActionHandler;

struct ERect {
    int x;
    int y;
    int width;
    int height;
};

class EventHandler {
public:
    EventHandler(ApplicationManager& appManager);
    ~EventHandler();

    void setupQuartzEventTap();
    void runPlatform();

    static void focusApplication(pid_t pid);

    ERect getLiveBoundsRect();

private:
    ApplicationManager& app_;
    LogHandler* log_;

    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;

};

#endif
