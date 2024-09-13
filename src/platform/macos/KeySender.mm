#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>
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
// Alphabet keys
const CGKeyCode kVK_ANSI_A = 0;
const CGKeyCode kVK_ANSI_S = 1;
const CGKeyCode kVK_ANSI_D = 2;
const CGKeyCode kVK_ANSI_F = 3;
const CGKeyCode kVK_ANSI_H = 4;
const CGKeyCode kVK_ANSI_G = 5;
const CGKeyCode kVK_ANSI_Z = 6;
const CGKeyCode kVK_ANSI_X = 7;
const CGKeyCode kVK_ANSI_C = 8;
const CGKeyCode kVK_ANSI_V = 9;
const CGKeyCode kVK_ANSI_B = 11;
const CGKeyCode kVK_ANSI_Q = 12;
const CGKeyCode kVK_ANSI_W = 13;
const CGKeyCode kVK_ANSI_E = 14;
const CGKeyCode kVK_ANSI_R = 15;
const CGKeyCode kVK_ANSI_Y = 16;
const CGKeyCode kVK_ANSI_T = 17;
const CGKeyCode kVK_ANSI_1 = 18;
const CGKeyCode kVK_ANSI_2 = 19;
const CGKeyCode kVK_ANSI_3 = 20;
const CGKeyCode kVK_ANSI_4 = 21;
const CGKeyCode kVK_ANSI_6 = 22;
const CGKeyCode kVK_ANSI_5 = 23;
const CGKeyCode kVK_ANSI_Equal = 24;
const CGKeyCode kVK_ANSI_9 = 25;
const CGKeyCode kVK_ANSI_7 = 26;
const CGKeyCode kVK_ANSI_Minus = 27;
const CGKeyCode kVK_ANSI_8 = 28;
const CGKeyCode kVK_ANSI_0 = 29;
const CGKeyCode kVK_ANSI_RightBracket = 30;
const CGKeyCode kVK_ANSI_O = 31;
const CGKeyCode kVK_ANSI_U = 32;
const CGKeyCode kVK_ANSI_LeftBracket = 33;
const CGKeyCode kVK_ANSI_I = 34;
const CGKeyCode kVK_ANSI_P = 35;
const CGKeyCode kVK_Return = 36;
const CGKeyCode kVK_ANSI_L = 37;
const CGKeyCode kVK_ANSI_J = 38;
const CGKeyCode kVK_ANSI_Quote = 39;
const CGKeyCode kVK_ANSI_K = 40;
const CGKeyCode kVK_ANSI_Semicolon = 41;
const CGKeyCode kVK_ANSI_Backslash = 42;
const CGKeyCode kVK_ANSI_Comma = 43;
const CGKeyCode kVK_ANSI_Slash = 44;
const CGKeyCode kVK_ANSI_N = 45;
const CGKeyCode kVK_ANSI_M = 46;
const CGKeyCode kVK_ANSI_Period = 47;
const CGKeyCode kVK_Tab = 48;
const CGKeyCode kVK_Space = 49;
const CGKeyCode kVK_ANSI_Grave = 50; // The tilde key (`~)
const CGKeyCode kVK_Delete = 51;
const CGKeyCode kVK_Escape = 53;

const CGKeyCode kVK_RightCommand = 54;
const CGKeyCode kVK_Command = 55;
const CGKeyCode kVK_Shift = 56;
const CGKeyCode kVK_CapsLock = 57;
const CGKeyCode kVK_Option = 58;
const CGKeyCode kVK_Control = 59;
const CGKeyCode kVK_RightShift = 60;
const CGKeyCode kVK_RightOption = 61;
const CGKeyCode kVK_RightControl = 62;

// Function keys
const CGKeyCode kVK_F1 = 122;
const CGKeyCode kVK_F2 = 120;
const CGKeyCode kVK_F3 = 99;
const CGKeyCode kVK_F4 = 118;
const CGKeyCode kVK_F5 = 96;
const CGKeyCode kVK_F6 = 97;
const CGKeyCode kVK_F7 = 98;
const CGKeyCode kVK_F8 = 100;
const CGKeyCode kVK_F9 = 101;
const CGKeyCode kVK_F10 = 109;
const CGKeyCode kVK_F11 = 103;
const CGKeyCode kVK_F12 = 111;

// Arrow keys
const CGKeyCode kVK_UpArrow = 126;
const CGKeyCode kVK_DownArrow = 125;
const CGKeyCode kVK_LeftArrow = 123;
const CGKeyCode kVK_RightArrow = 124;

// Other special keys
const CGKeyCode kVK_Home = 115;
const CGKeyCode kVK_End = 119;
const CGKeyCode kVK_PageUp = 116;
const CGKeyCode kVK_PageDown = 121;
const CGKeyCode kVK_Help = 114;
const CGKeyCode kVK_ForwardDelete = 117;


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
        log_->info("KeySender:: Keypress sent: "  + kp.key                   );

        CGEventFlags flags = getEventFlags(kp);
        CGKeyCode keyCode = 0;

        char character = kp.key[0];
        // Map character to key code and modifier flags
        if (character >= 'a' && character <= 'z') {
            keyCode = kVK_ANSI_A + (character - 'a');
        } else if (character >= 'A' && character <= 'Z') {
            keyCode = kVK_ANSI_A + (character - 'A');
        } else if (character >= '0' && character <= '9') {
            keyCode = kVK_ANSI_0 + (character - '0');
        } else {
            // Add additional mappings for special characters
            switch (character) {
                case ' ':
                    keyCode = kVK_Space;
                    break;
                case '\n':
                    keyCode = kVK_Return;
                    break;
                case '\t':
                    keyCode = kVK_Tab;
                    break;
                case '.':
                    keyCode = kVK_ANSI_Period;
                    break;
                case '-':
                    keyCode = kVK_ANSI_Minus;
                    break;
                // Add other cases as needed
                default:
                    return; // Unsupported character
            }
        }

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
