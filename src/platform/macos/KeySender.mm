#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>
#include <unordered_map>
#include <string>

#include "ApplicationManager.h"
#include "KeySender.h"
#include "KeyMapper.h"
#include "PID.h"

#include "Types.h"


// TODO
//CGKeyCode getKeyCodeFromCharacter(char character) {
//    switch (character) {
//        case 'a': return kVK_ANSI_A;
//        case 'b': return kVK_ANSI_B;
//        case 'c': return kVK_ANSI_C;
//        case '-': return kVK_ANSI_Minus;
//        case '1': return kVK_ANSI_1;
//        // Add other mappings as needed
//        default: return -1; // Invalid key
//    }
//}
std::unordered_map<std::string, CGKeyCode> keyCodeMap = {
    {"a", 0}
    , {"s", 1}
    , {"d", 2}
    , {"f", 3}
    , {"h", 4}
    , {"g", 5}
    , {"z", 6}
    , {"x", 7}
    , {"c", 8}
    , {"v", 9}
    , {"b", 11}
    , {"q", 12}
    , {"w", 13}
    , {"e", 14}
    , {"r", 15}
    , {"y", 16}
    , {"t", 17}
    , {"1", 18}
    , {"2", 19}
    , {"3", 20}
    , {"4", 21}
    , {"6", 22}
    , {"5", 23}
    , {"Equal", 24}
    , {"9", 25}
    , {"7", 26}
    , {"Minus", 27}
    , {"8", 28}
    , {"0", 29}
    , {"RightBracket", 30}
    , {"o", 31}
    , {"u", 32}
    , {"LeftBracket", 33}
    , {"i", 34}
    , {"p", 35}
    , {"n", 36}
    , {"l", 37}
    , {"j", 38}
    , {"Quote", 39}
    , {"k", 40}
    , {"Semicolon", 41}
    , {"Backslash", 42}
    , {"Comma", 43}
    , {"Slash", 44}
    , {"n", 45}
    , {"m", 46}
    , {"Period", 47}
    , {"Tab", 48}
    , {"Space", 49}
    , {"Grave", 50}
    , {"Delete", 51}
    , {"Escape", 53}

    , {"RightCommand", 54}
    , {"Command", 55}
    , {"Shift", 56}
    , {"CapsLock", 57}
    , {"Option", 58}
    , {"Control", 59}
    , {"RightShift", 60}
    , {"RightOption", 61}
    , {"RightControl", 62}

    , {"F1", 122}
    , {"F2", 120}
    , {"F3", 99}
    , {"F4", 118}
    , {"F5", 96}
    , {"F6", 97}
    , {"F7", 98}
    , {"F8", 100}
    , {"F9", 101}
    , {"F10", 109}
    , {"F11", 103}
    , {"F12", 111}

    , {"UpArrow", 126}
    , {"DownArrow", 125}
    , {"LeftArrow", 123}
    , {"RightArrow", 124}

    , {"Home", 115}
    , {"End", 119}
    , {"PageUp", 116}
    , {"PageDown", 121}
    , {"Help", 114}
    , {"ForwardDelete", 117}
};

CGKeyCode getKeyCode(const std::string& key) {
    auto it = keyCodeMap.find(key);
    if (it != keyCodeMap.end()) {
        return it->second;
    }
    return 0;  // Default value if key not found
}

KeySender::KeySender(ApplicationManager& appManager)
    : app_(appManager)
    , log_(appManager.getLogHandler())
    , keyMapper_(new KeyMapper())
{}

KeySender::~KeySender() {}

CGEventFlags getEventFlags(const EKeyPress& kp) {
    CGEventFlags flags = 0;

    if (kp.cmd) {
        flags |= kCGEventFlagMaskCommand;
    }
    if (kp.ctrl) {
        flags |= kCGEventFlagMaskControl;
    }
    if (kp.alt) {
        flags |= kCGEventFlagMaskAlternate;
    }
    if (kp.shift) {
        flags |= kCGEventFlagMaskShift;
    }

    return flags;
}

void KeySender::sendKeyDown(const EKeyPress& kp) {
    const CGKeyCode kVK_RightCommand = 54;
    const CGKeyCode kVK_Command = 55;
    const CGKeyCode kVK_Shift = 56;
    const CGKeyCode kVK_CapsLock = 57;
    const CGKeyCode kVK_Option = 58;
    const CGKeyCode kVK_Control = 59;
    const CGKeyCode kVK_RightShift = 60;
    const CGKeyCode kVK_RightOption = 61;
    const CGKeyCode kVK_RightControl = 62;


}

void KeySender::sendKeyUp(const EKeyPress& kp) {

}

void KeySender::sendIndividualKeyPress(const EKeyPress& kp) {

}

void KeySender::sendModifiedKeyCombo(const EKeyPress& kp) {

}

void KeySender::sendKeyPress(const EKeyPress& kp) {
    dispatch_async(dispatch_get_main_queue(), ^{
        log_->info("KeySender:: Keypress cmd: "   + std::to_string(kp.cmd)   );
        log_->info("KeySender:: Keypress ctrl: "  + std::to_string(kp.ctrl)  );
        log_->info("KeySender:: Keypress alt: "   + std::to_string(kp.alt)   );
        log_->info("KeySender:: Keypress shift: " + std::to_string(kp.shift) );
        log_->info("KeySender:: Keypress key: "   + kp.key                   );

        CGEventFlags flags = getEventFlags(kp);

        CGKeyCode keyCode = getKeyCode(kp.key);
        // Map character to key code and modifier flags

        log_->info("KeySender:: keycode: " + std::to_string(keyCode));

        CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, keyCode, true);
        CGEventSetFlags(keyDown, flags);
        CGEventPost(kCGAnnotatedSessionEventTap, keyDown);
        CFRelease(keyDown);

        //log_->info("sending keyUp: " + std::to_string(keyCode));
        CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, keyCode, false);
        CGEventSetFlags(keyUp, flags);
        CGEventPost(kCGAnnotatedSessionEventTap, keyDown);
        CFRelease(keyUp);
    });
}
