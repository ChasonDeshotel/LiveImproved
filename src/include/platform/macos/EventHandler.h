#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <JuceHeader.h>
#include <memory>
#include <functional>

#include <ApplicationServices/ApplicationServices.h>

#include "Types.h"

class ILogHandler;
class IActionHandler;
class WindowManager;
class PID;

class EventHandler {
public:
    EventHandler(
        std::function<std::shared_ptr<ILogHandler>()> logHandler
        , std::function<std::shared_ptr<IActionHandler>()> actionHandler
        , std::function<std::shared_ptr<WindowManager>()> windowManager
    );
    ~EventHandler();

    void setupQuartzEventTap();
    void runPlatform();

    static void focusLim();
    static void focusLive();
    static void focusWindow(void* nativeWindowHandle);

    ERect getLiveBoundsRect();

private:
    std::function<std::shared_ptr<ILogHandler>()> log_;
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;

    static void focusApplication(pid_t pid);

    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;

};

#endif
