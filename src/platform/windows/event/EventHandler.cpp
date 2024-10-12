#include <windows.h>
#include <string>
#include <iostream>
#include <optional>
#include <chrono>

#include "LogHandler.h"
#include "EventHandler.h"
#include "Types.h"
#include "PID.h"
#include "WindowManager.h"
#include "ActionHandler.h"

#include <Windows.h>
#include <string>

// TODO move... somewhere
std::string keyCodeToString(DWORD keyCode) {
    // Handle special keys
    switch (keyCode) {
        // Function keys
        case VK_F1:  return "F1";
        case VK_F2:  return "F2";
        case VK_F3:  return "F3";
        case VK_F4:  return "F4";
        case VK_F5:  return "F5";
        case VK_F6:  return "F6";
        case VK_F7:  return "F7";
        case VK_F8:  return "F8";
        case VK_F9:  return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";

            // Arrow keys
        case VK_LEFT:  return "Left Arrow";
        case VK_RIGHT: return "Right Arrow";
        case VK_DOWN:  return "Down Arrow";
        case VK_UP:    return "Up Arrow";

            // Other special keys
        case VK_ESCAPE:   return "Escape";
        case VK_TAB:      return "Tab";
        case VK_SPACE:    return "Space";
        case VK_RETURN:   return "Return";
        case VK_DELETE:   return "Delete";
        case VK_HOME:     return "Home";
        case VK_END:      return "End";
        case VK_PRIOR:    return "Page Up";
        case VK_NEXT:     return "Page Down";

            // Other keys
        case VK_OEM_PLUS:     return "+";
        case VK_OEM_MINUS:    return "-";
        case VK_OEM_3:        return "`";
        case VK_OEM_4:        return "[";
        case VK_OEM_6:        return "]";
        case VK_OEM_5:        return "\\";

        default: {
            // Map virtual key code to the corresponding character using MapVirtualKey
            // for non-special keys like letters and numbers
            char buffer[2] = { 0 };
            UINT scanCode = MapVirtualKey(keyCode, MAPVK_VK_TO_VSC);
            if (scanCode) {
                // Try to convert the virtual key to a character
                if (GetKeyNameTextA(scanCode << 16, buffer, sizeof(buffer)) > 0) {
                    return std::string(buffer);
                }
            }
            return "[Unknown Key]";
        }
    }
}

HHOOK keyboardHook = NULL;
HHOOK mouseHook = NULL;

EventHandler* EventHandler::instance = nullptr;

EventHandler::EventHandler(WindowManager& windowManager, ActionHandler& actionHandler)
    : windowManager_(windowManager)
    , actionHandler_(actionHandler)
    , log_(LogHandler::getInstance()) {
    instance = this;
}

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

std::optional<std::chrono::steady_clock::time_point> lastRightClickTime;
// TODO: move to config
const int doubleClickThresholdMs = 300;

LRESULT CALLBACK EventHandler::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        DWORD livePID = PID::getInstance().livePID();  // Get Ableton Live PID

        // Check for mouse down events
        if (wParam == WM_RBUTTONDOWN || wParam == WM_LBUTTONDOWN) {
            LogHandler::getInstance().debug("Mouse down event detected");

            // Check if search box is open
            if (instance->windowManager_.isWindowOpen("SearchBox")) {
                HWND searchBoxHwnd = static_cast<HWND>(instance->windowManager_.getWindowHandle("SearchBox"));
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
                        instance->windowManager_.closeWindow("SearchBox");
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
                        instance->actionHandler_.handleDoubleRightClick();
                        return 1;  // Stop further processing
                    }
                }
                lastRightClickTime = now;
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK EventHandler::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;

        if (wParam == WM_KEYDOWN) {
            LogHandler::getInstance().debug("Key down event detected in Ableton Live.");

            // TODO cross platform
            #ifndef _WIN32
				// Pass event if any text field in Ableton Live has focus
				if (liveInterface->isAnyTextFieldFocused()) {
					LogHandler::getInstance().debug("Ableton Live text field has focus, passing event.");
					return CallNextHookEx(NULL, nCode, wParam, lParam);
				}
			#endif

            // Handle key events
            // TODO cross platform send EKeyPress
            DWORD vkCode = kbdStruct->vkCode;
            std::string keyString = keyCodeToString(vkCode);
            EKeyPress pressedKey;
            pressedKey.shift = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
            pressedKey.ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            pressedKey.cmd   = (GetAsyncKeyState(VK_LWIN)    & 0x8000) != 0;
            pressedKey.alt   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
            pressedKey.key   = keyCodeToString(kbdStruct->vkCode);
            pressedKey.state = KeyState::Down;
            bool shouldPassEvent = instance->actionHandler_.handleKeyEvent(pressedKey);

            if (!shouldPassEvent) {
                return 1;  // Block the event if not to be passed
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL CALLBACK EventHandler::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
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
    focusApplication(PID::getInstance().appPID());
}

void EventHandler::focusLive() {
    focusApplication(PID::getInstance().livePID());
}

void EventHandler::focusApplication(pid_t pid) {
    HWND hwnd = NULL;
    DWORD pidDWORD = static_cast<DWORD>(pid); 

    // Enumerate all windows to find the one that matches the given PID
    EnumWindows(EnumWindowsProc, (LPARAM)&pidDWORD);

    if (hwnd != NULL) {
        log_.debug("Bringing app into focus: " + std::to_string(pidDWORD));

        // Restore and bring the window to the foreground
        ShowWindow(hwnd, SW_RESTORE);  // Restore if minimized
        SetForegroundWindow(hwnd);     // Bring to foreground
    } else {
        log_.error("Application window not found with PID: " + std::to_string(pidDWORD));
    }
}

ERect EventHandler::getLiveBoundsRect() {
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
