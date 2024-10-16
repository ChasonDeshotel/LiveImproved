#pragma once

#include <functional>

class ERect;
class IActionHandler;
class WindowManager;

class IEventHandler {
public:
    virtual ~IEventHandler() = default;

    IEventHandler(const IEventHandler &) = default;
    IEventHandler(IEventHandler &&) = delete;
    auto operator=(const IEventHandler &) -> IEventHandler & = default;
    auto operator=(IEventHandler &&) -> IEventHandler & = delete;
    
    virtual auto runPlatform() -> void = 0;
    virtual auto setupQuartzEventTap() -> void = 0;
    virtual auto registerAppLaunch(std::function<void()> onLaunchCallback) -> void = 0;
    virtual auto registerAppTermination(std::function<void()> onTerminationCallback) -> void = 0;

    #ifdef _WIN32
//    virtual auto setupWindowsEventHook() -> void = 0;
//    virtual auto cleanupWindowsHooks() -> void = 0;
    #endif

    virtual auto focusLim()    -> void = 0;
    virtual auto focusLive()   -> void = 0;
    virtual auto focusWindow(void *nativeWindowHandle) -> void = 0;
    virtual auto focusWindow(int windowID) -> void = 0;

    virtual auto getLiveBoundsRect() -> ERect = 0;


protected:
    IEventHandler() = default;
};
