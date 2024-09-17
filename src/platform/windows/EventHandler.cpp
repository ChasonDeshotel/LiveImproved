#include <windows.h>
#include <string>
#include <iostream>
#include <optional>

#include "EventHandler.h"
#include "Types.h"

HHOOK keyboardHook = NULL;
HHOOK mouseHook = NULL;

EventHandler::EventHandler(WindowManager& windowManager, ActionHandler& actionHandler)
    : windowManager_(windowManager)
    , actionHandler_(actionHandler)
    , log_(LogHandler::getInstance()) {}

EventHandler::~EventHandler() {
    cleanupWindowsHooks();
}

struct MouseLocationAndBounds {
    POINT mouseLocation;
    RECT liveBounds;
    RECT appBounds;
};

void EventHandler::setupWindowsEventHook() {
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);

    if (!keyboardHook || !mouseHook) {
        log_.error("Failed to create hooks for event handling");
        return;
    }

    log_.debug("EventHandler: Windows hooks are active!");
}

void EventHandler::cleanupWindowsHooks() {
    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
    }
    if (mouseHook) {
        UnhookWindowsHookEx(mouseHook);
    }
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        DWORD livePID = PID::getInstance().livePID();  // Get Ableton Live PID

        // Check for mouse down events
        if (wParam == WM_RBUTTONDOWN || wParam == WM_LBUTTONDOWN) {
            LogHandler::getInstance().debug("Mouse down event detected");

            // Check if search box is open
            if (WindowManager::getInstance().isWindowOpen("SearchBox")) {
                HWND searchBoxHwnd = WindowManager::getInstance().getWindowHandle("SearchBox");
                RECT appBounds;
                GetWindowRect(searchBoxHwnd, &appBounds);

                // Get mouse location
                POINT mouseLocation = mouseStruct->pt;

                // Check if the click is inside the app window or outside
                if (!PtInRect(&appBounds, mouseLocation)) {
                    // Now check if the click is inside Ableton Live's window
                    HWND liveHwnd = FindWindow(NULL, NULL);  // Replace with actual title/class if needed
                    RECT liveBounds;
                    GetWindowRect(liveHwnd, &liveBounds);

                    if (PtInRect(&liveBounds, mouseLocation)) {
                        LogHandler::getInstance().debug("Click is outside app but inside Live, closing search box.");
                        WindowManager::getInstance().closeWindow("SearchBox");
                    }
                } else {
                    LogHandler::getInstance().debug("Click is inside the search box window, keeping window open.");
                }
            }

            // Double-right-click detection
            if (wParam == WM_RBUTTONDOWN) {
                auto now = std::chrono::steady_clock::now();
                if (lastRightClickTime.has_value()) {
                    auto durationSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRightClickTime.value());
                    if (durationSinceLastClick.count() <= doubleClickThresholdMs) {
                        LogHandler::getInstance().debug("Double-right-click detected, handling action.");
                        ActionHandler::getInstance().handleDoubleRightClick();
                        return 1;  // Stop further processing
                    }
                }
                lastRightClickTime = now;
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;

        if (wParam == WM_KEYDOWN) {
            LogHandler::getInstance().debug("Key down event detected in Ableton Live.");

            // Pass event if any text field in Ableton Live has focus
            if (liveInterface->isAnyTextFieldFocused()) {
                LogHandler::getInstance().debug("Ableton Live text field has focus, passing event.");
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }

            // Handle key events
            DWORD vkCode = kbdStruct->vkCode;
            std::string keyString = keyCodeToString(vkCode);
            bool shouldPassEvent = ActionHandler::getInstance().handleKeyEvent(keyString, 0, "keyDown");

            if (!shouldPassEvent) {
                return 1;  // Block the event if not to be passed
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD targetPID = *(DWORD*)lParam;
    DWORD windowPID = 0;
    GetWindowThreadProcessId(hwnd, &windowPID);

    if (windowPID == targetPID) {
        *(HWND*)lParam = hwnd;  // Assign the window handle to lParam
        return FALSE;  // Stop enumeration
    }
    return TRUE;  // Continue enumeration
}

void EventHandler::focusLim() {
    EventHandler::focusApplication(PID::getInstance().appPID());
}

void EventHandler::focusLive() {
    EventHandler::focusApplication(PID::getInstance().livePID());
}

void EventHandler::focusApplication(DWORD pid) {
    HWND hwnd = NULL;

    // Enumerate all windows to find the one that matches the given PID
    EnumWindows(EnumWindowsProc, (LPARAM)&pid);

    if (hwnd != NULL) {
        log_.debug("Bringing app into focus: " + std::to_string(pid));

        // Restore and bring the window to the foreground
        ShowWindow(hwnd, SW_RESTORE);  // Restore if minimized
        SetForegroundWindow(hwnd);     // Bring to foreground
    } else {
        log_.error("Application window not found with PID: " + std::to_string(pid));
    }
}

ERect getLiveBounds() {
    ERect appBounds = { 0, 0, 0, 0 };  // Initialize to empty bounds
    DWORD livePID = PID::getInstance().livePID();  // Get the PID of Ableton Live

    // Enumerate all top-level windows to find the one that matches the live PID
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        DWORD windowPID = 0;
        GetWindowThreadProcessId(hwnd, &windowPID);

        // Check if this window belongs to the process with livePID
        if (windowPID == *(DWORD*)lParam) {
            RECT rect;
            if (GetWindowRect(hwnd, &rect)) {
                ERect* appBounds = reinterpret_cast<ERect*>(lParam);
                appBounds->x = rect.left;
                appBounds->y = rect.top;
                appBounds->width = rect.right - rect.left;
                appBounds->height = rect.bottom - rect.top;
                return FALSE;  // Stop enumeration as we've found the window
            }
        }
        return TRUE;  // Continue enumeration
        }, (LPARAM)&livePID);

    return appBounds;  // Return the bounds
}
