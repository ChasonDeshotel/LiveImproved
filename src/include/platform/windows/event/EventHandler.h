#pragma once

#include <windows.h>
#include <string>
#include <optional>

#include "Types.h"

class ActionHandler;
class WindowManager;
class PID;

class EventHandler {
public:
    EventHandler(WindowManager& windowManager, ActionHandler& actionHandler);
    ~EventHandler();

    void setupWindowsEventHook();
    void cleanupWindowsHooks();

    void focusLim();
    void focusLive();

    ERect getLiveBoundsRect();

    static EventHandler* instance;

private:
    WindowManager& windowManager_;
    ActionHandler& actionHandler_;

    void focusApplication(pid_t pid);

    // Hook callback functions
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK keyboardHook;
    HHOOK mouseHook;

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
};
