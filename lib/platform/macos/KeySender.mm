#include <ApplicationServices/ApplicationServices.h>
#include <string>

#include "KeySender.h"
#include "../../ApplicationManager.h"

KeySender::KeySender(ApplicationManager& appManager)
    : app_(appManager)
{}

KeySender::~KeySender() {}

void KeySender::sendKeypress(int keyCode, int flags) {
    app_.getLogHandler()->info("sending keydown: " + std::to_string(keyCode));

    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, true);
    CGEventSetFlags(keyDown, (CGEventFlags)flags);
    CGEventPost(kCGAnnotatedSessionEventTap, keyDown);
    CFRelease(keyDown);

    app_.getLogHandler()->info("sending keyup: " + std::to_string(keyCode));
    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, false);
    CGEventSetFlags(keyUp, (CGEventFlags)flags);
    CGEventPost(kCGAnnotatedSessionEventTap, keyUp);
    CFRelease(keyUp);
}
