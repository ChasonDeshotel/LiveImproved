#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include <string>

#ifdef _WIN32
#include "platform/win/KeySender.h"
#else
#include "platform/macos/KeySender.h"
#endif

class ActionHandler {
public:
    ActionHandler();
    ~ActionHandler();

    // returns if the event should be blocking
    bool handleKeyEvent(int keyCode, int flags, std::string type);

private:
    KeySender keySender;

    bool onEscapePress();
};

#endif
