#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <ApplicationServices/ApplicationServices.h>

class LogHandler;
class ActionHandler;
class WindowManager;
class PID;

struct ERect {
    int x;
    int y;
    int width;
    int height;
};

class EventHandler {
public:
    EventHandler(WindowManager& windowManager, ActionHandler& actionHandler);
    ~EventHandler();

    static void focusLim();
    static void focusLive();

    ERect getLiveBoundsRect();

private:
    WindowManager& windowManager_;
    ActionHandler& actionHandler_;
    LogHandler& log_;

    static void focusApplication(pid_t pid);

    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;

};

#endif
