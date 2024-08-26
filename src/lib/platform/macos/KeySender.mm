#include <ApplicationServices/ApplicationServices.h>
#include <string>

#include "ApplicationManager.h"
#include "KeySender.h"

KeySender::KeySender(ApplicationManager& appManager)
    : log_(appManager.getLogHandler())
{}

KeySender::~KeySender() {}

void KeySender::sendKeyPress(int keyCode, int flags) {
    log_->info("sending keydown: " + std::to_string(keyCode));

    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, true);
    CGEventSetFlags(keyDown, (CGEventFlags)flags);
    CGEventPost(kCGAnnotatedSessionEventTap, keyDown);
    CFRelease(keyDown);

    log_->info("sending keyup: " + std::to_string(keyCode));
    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)keyCode, false);
    CGEventSetFlags(keyUp, (CGEventFlags)flags);
    CGEventPost(kCGAnnotatedSessionEventTap, keyUp);
    CFRelease(keyUp);
}
