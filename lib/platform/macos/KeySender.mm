#include <ApplicationServices/ApplicationServices.h>
#include <string>
#include "KeySender.h"
#include "../../LogHandler.h"

void KeySender::sendKeypress(int keyCode, int flags) {
    LogHandler::getInstance().info("sending keydown: " + std::to_string(keyCode));

    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, true);
    CGEventSetFlags(keyDown, (CGEventFlags)flags);
    CGEventPost(kCGAnnotatedSessionEventTap, keyDown);
    CFRelease(keyDown);

    LogHandler::getInstance().info("sending keyup: " + std::to_string(keyCode));
    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, false);
    CGEventSetFlags(keyUp, (CGEventFlags)flags);
    CGEventPost(kCGAnnotatedSessionEventTap, keyUp);
    CFRelease(keyUp);
}
