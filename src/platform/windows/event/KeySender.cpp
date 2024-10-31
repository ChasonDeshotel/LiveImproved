#include <windows.h>
#include <unordered_map>
#include <string>
#include <optional>
#include <algorithm>

#include "LogGlobal.h"
#include "Types.h"

#include "IKeySender.h"
#include "KeySender.h"

KeySender::KeySender() = default;
KeySender::~KeySender() = default;

namespace {
    std::unordered_map<std::string, WORD> keyCodeMap = {
        {"a", 0x41}, {"s", 0x53}, {"d", 0x44}, {"f", 0x46}, {"h", 0x48},
        {"g", 0x47}, {"z", 0x5A}, {"x", 0x58}, {"c", 0x43}, {"v", 0x56},
        {"b", 0x42}, {"q", 0x51}, {"w", 0x57}, {"e", 0x45}, {"r", 0x52},
        {"y", 0x59}, {"t", 0x54}, {"1", 0x31}, {"2", 0x32}, {"3", 0x33},
        {"4", 0x34}, {"5", 0x35}, {"6", 0x36}, {"7", 0x37}, {"8", 0x38},
        {"9", 0x39}, {"0", 0x30}, {"tab", VK_TAB}, {"space", VK_SPACE},
        {"escape", VK_ESCAPE}, {"left", VK_LEFT}, {"right", VK_RIGHT},
        {"up", VK_UP}, {"down", VK_DOWN}, {"home", VK_HOME}, {"end", VK_END},
        {"pageup", VK_PRIOR}, {"pagedown", VK_NEXT}, {"f1", VK_F1}, {"f2", VK_F2},
        {"f3", VK_F3}, {"f4", VK_F4}, {"f5", VK_F5}, {"f6", VK_F6}, {"f7", VK_F7},
        {"f8", VK_F8}, {"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12}
    };

    std::string toLower(const std::string& str) {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return lowerStr;
    }

    std::optional<WORD> getKeyCode(const std::string& key) {
        auto it = keyCodeMap.find(key);
        if (it != keyCodeMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void sendInputKey(WORD keyCode, bool keyDown) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = keyCode;
        input.ki.dwFlags = (keyDown) ? 0 : KEYEVENTF_KEYUP;

        SendInput(1, &input, sizeof(INPUT));
    }

    void pressModifierKeys(const EKeyPress& kp, bool keyDown) {
        if (kp.ctrl) sendInputKey(VK_CONTROL, keyDown);
        if (kp.alt) sendInputKey(VK_MENU, keyDown);  // Alt key
        if (kp.shift) sendInputKey(VK_SHIFT, keyDown);
        if (kp.cmd) sendInputKey(VK_LWIN, keyDown);  // Windows key (cmd equivalent)
    }
}

void sendIndividualKeyPress(const EKeyPress& kp) {
    std::optional<WORD> keyCodeOpt = getKeyCode(toLower(kp.key));
    if (keyCodeOpt) {
        WORD keyCode = *keyCodeOpt;

        // Press down the modifier keys
        pressModifierKeys(kp, true);

        // Press and release the main key
        sendInputKey(keyCode, true);  // Key down
        sendInputKey(keyCode, false); // Key up

        // Release the modifier keys
        pressModifierKeys(kp, false);
    }
}

void KeySender::sendKeyPress(const EKeyPress& kpRef) {
    EKeyPress kp = kpRef; // create a copy
    logger->debug("KeySender::sendKeyPress called");

    logger->debug("KeySender:: Keypress cmd: "   + std::to_string(kp.cmd));
    logger->debug("KeySender:: Keypress ctrl: "  + std::to_string(kp.ctrl));
    logger->debug("KeySender:: Keypress alt: "   + std::to_string(kp.alt));
    logger->debug("KeySender:: Keypress shift: " + std::to_string(kp.shift));
    logger->debug("KeySender:: Keypress key: "   + kp.key);

    sendIndividualKeyPress(kp);
}
