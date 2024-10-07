#pragma once

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>

#include <JuceHeader.h>
#include <memory>
#include <functional>

#include "Types.h"

#ifdef __OBJC__
    @class NSView;
    @class NSWindow;
#else
    // Forward declare as empty structs for pure C++ compatibility
    struct NSView;
    struct NSWindow;
#endif

class ILogHandler;
class IActionHandler;
class WindowManager;

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
    static void focusWindow(int windowID);

    NSView* getViewFromWindowID(int windowID);
    NSWindow* getWindowFromWindowID(int windowID);

    ERect getLiveBoundsRect();

    void registerAppLaunch(std::function<void()> onLaunchCallback);
    void registerAppTermination(std::function<void()> onTerminationCallback);

private:
    std::function<std::shared_ptr<ILogHandler>()> log_;
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;

    static void focusApplication(pid_t pid);

    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;

};
