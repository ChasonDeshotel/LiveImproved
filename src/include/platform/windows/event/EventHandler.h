#pragma once

#include <Windows.h>
#include <string>
#include <optional>

#include "IEventHandler.h"
#include "Types.h"

class IActionHandler;
class WindowManager;

class EventHandler : public IEventHandler {
public:
    EventHandler(std::function<std::shared_ptr<IActionHandler>()> actionHandler
                 , std::function<std::shared_ptr<WindowManager>()> windowManager
    );

    ~EventHandler() override;

    EventHandler(const EventHandler &) = default;
    EventHandler(EventHandler &&) = delete;
    EventHandler &operator=(const EventHandler &) = delete;
    EventHandler &operator=(EventHandler &&) = default;

    void focusLim() override;
    void focusLive() override;
    void focusWindow(void* nativeWindowHandle) override;
    void focusWindow(int windowID) override;

    ERect getLiveBoundsRect() override;

    void runPlatform() override;
    void setupQuartzEventTap() override;

    void registerAppLaunch(std::function<void()> onLaunchCallback) override;
    void registerAppTermination(std::function<void()> onLaunchCallback) override;

private:
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;

    void setupWindowsEventHook();
    void cleanupWindowsHooks();

    void focusApplication(pid_t pid);

    // Hook callback functions
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK keyboardHook_;
    HHOOK mouseHook_;

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
};
