#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>
#include <unordered_map>
#include <string>
#include <optional>

#include "LogHandler.h"

#include "Types.h"

#include "KeySender.h"

std::string toLower(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

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
    , {"equal", 24}
    , {"9", 25}
    , {"7", 26}
    , {"minus", 27}
    , {"8", 28}
    , {"0", 29}
    , {"rightbracket", 30}
    , {"o", 31}
    , {"u", 32}
    , {"leftbracket", 33}
    , {"i", 34}
    , {"p", 35}
    , {"n", 36}
    , {"l", 37}
    , {"j", 38}
    , {"quote", 39}
    , {"k", 40}
    , {"semicolon", 41}
    , {"backslash", 42}
    , {"comma", 43}
    , {"slash", 44}
    , {"n", 45}
    , {"m", 46}
    , {"period", 47}
    , {"tab", 48}
    , {"space", 49}
    , {"grave", 50}
    , {"delete", 51}
    , {"escape", 53}

    , {"rightcommand", 54}
    , {"command", 55}
    , {"shift", 56}
    , {"capslock", 57}
    , {"option", 58}
    , {"control", 59}
    , {"rightshift", 60}
    , {"rightoption", 61}
    , {"rightcontrol", 62}

    , {"f1", 122}
    , {"f2", 120}
    , {"f3", 99}
    , {"f4", 118}
    , {"f5", 96}
    , {"f6", 97}
    , {"f7", 98}
    , {"f8", 100}
    , {"f9", 101}
    , {"f10", 109}
    , {"f11", 103}
    , {"f12", 111}

    , {"uparrow", 126}
    , {"downarrow", 125}
    , {"leftarrow", 123}
    , {"rightarrow", 124}

    , {"home", 115}
    , {"end", 119}
    , {"pageup", 116}
    , {"pagedown", 121}
    , {"help", 114}
    , {"forwarddelete", 117}
};

std::optional<CGKeyCode> getKeyCode(const std::string& key) {
    auto it = keyCodeMap.find(key);
    if (it != keyCodeMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

KeySender::KeySender()
    : log_(&LogHandler::getInstance())
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

}

void KeySender::sendKeyUp(const EKeyPress& kp) {

}

void KeySender::sendIndividualKeyPress(const EKeyPress& kp) {

}

void KeySender::sendModifiedKeyCombo(const EKeyPress& kp) {

}

void KeySender::sendKeyPress(const EKeyPress& kpRef) {
    EKeyPress kp = kpRef; // create a copy

    dispatch_async(dispatch_get_main_queue(), ^{
        CGEventFlags flags = getEventFlags(kp);
        std::optional<CGKeyCode> keyCodeOpt = getKeyCode(toLower(kp.key));

        log_->info("KeySender:: Keypress cmd: "   + std::to_string(kp.cmd)   );
        log_->info("KeySender:: Keypress ctrl: "  + std::to_string(kp.ctrl)  );
        log_->info("KeySender:: Keypress alt: "   + std::to_string(kp.alt)   );
        log_->info("KeySender:: Keypress shift: " + std::to_string(kp.shift) );
        log_->info("KeySender:: Keypress key: "   + kp.key                   );

        if (keyCodeOpt) {
            CGKeyCode keyCode = *keyCodeOpt;
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
        }
    });
}
