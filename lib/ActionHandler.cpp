#include "ActionHandler.h"
#include "ApplicationManager.h"
#include "Log.h"

ActionHandler::ActionHandler() : keySender() {
}

// returns a bool that tells the event handler 
// whether or not to block the event
bool ActionHandler::handleKeyEvent(int keyCode, int flags, std::string type) {
    if (type == "keyDown") {
        switch (keyCode) {
            case 53:  // escape
                return onEscapePress();
            default:
                return false;
        }
    }

    // if we meet no criteria,
    // the original event should
    // not be blocked
    return false;
}

bool ActionHandler::onEscapePress() {
    Log::logToFile("Escape pressed");
    return true;
}

