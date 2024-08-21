#include "KeySender.h"
#include <ApplicationServices/ApplicationServices.h>

void KeySender::sendKeypress(int keyCode, int flags) {
    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, true);
    CGEventSetFlags(keyDown, (CGEventFlags)flags);
    CGEventPost(kCGAnnotatedSessionEventTap, keyDown);
    CFRelease(keyDown);

    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, false);
    CGEventSetFlags(keyUp, (CGEventFlags)flags);
    CGEventPost(kCGAnnotatedSessionEventTap, keyUp);
    CFRelease(keyUp);
}
