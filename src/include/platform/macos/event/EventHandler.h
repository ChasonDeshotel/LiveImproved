#pragma once

#ifdef __OBJC__
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#elif _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <memory>
#include <functional>

#include "Types.h"
#include "IEventHandler.h"
#include "ILiveInterface.h"

#ifdef __OBJC__
@class NSView;
@class NSWindow;
@class NSDictionary;
@class NSArray;
#else
// Forward declare as empty structs for pure C++ compatibility
struct NSView;
struct NSWindow;
struct NSDictionary;
struct NSArray;
#endif

class IActionHandler;
class WindowManager;

class EventHandler : public IEventHandler {
public:
    EventHandler(std::function<std::shared_ptr<IActionHandler>()> actionHandler
                 , std::function<std::shared_ptr<WindowManager>()> windowManager
                 , std::function<std::shared_ptr<ILiveInterface>()> liveInterface
    );

    ~EventHandler() override;

    EventHandler(const EventHandler &) = default;
    EventHandler(EventHandler &&) = delete;
    EventHandler &operator=(const EventHandler &) = default;
    EventHandler &operator=(EventHandler &&) = delete;

    #ifndef _WIN32
    void runPlatform() override;
    void setupQuartzEventTap() override;
    void registerAppLaunch(std::function<void()> onLaunchCallback) override;
    void registerAppTermination(std::function<void()> onTerminationCallback) override;
    #endif

    void focusLim() override;
    void focusLive() override;
    void focusWindow(void *nativeWindowHandle) override;
    void focusWindow(int windowID) override;

    ERect getLiveBoundsRect() override;

    auto initializeCache() -> void;

private:
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<WindowManager>()>  windowManager_;
    std::function<std::shared_ptr<ILiveInterface>()> liveInterface_;

    WindowManager* resolvedWindowManager_;
    IActionHandler* resolvedActionHandler_;
    ILiveInterface* resolvedLiveInterface_;

    static void focusApplication(pid_t pid);

    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;

    bool isWindowFocused(int windowID);
    void test();

    NSView *getViewFromWindowID(int windowID);
    NSWindow *getWindowFromWindowID(int windowID);
    NSArray *getAllWindowsForApp();
    NSDictionary *getMainWindowForApp(NSArray *appWindows);
    void logWindowInfo(NSDictionary *windowInfo);
};
