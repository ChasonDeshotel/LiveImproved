#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <windows.h>
#include <string>
#include <optional>

#include "Types.h"

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

    void setupWindowsEventHook();
    void cleanupWindowsHooks();

    static void focusLim();
    static void focusLive();

    ERect getLiveBoundsRect();

private:
    WindowManager& windowManager_;
    ActionHandler& actionHandler_;
    LogHandler& log_;

    static void focusApplication(pid_t pid);

    // Hook callback functions
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK keyboardHook;
    HHOOK mouseHook;

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
};

#endif  // EVENT_HANDLER_H
