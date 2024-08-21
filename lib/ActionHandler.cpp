#include "ActionHandler.h"
#include "ApplicationManager.h"
#include "Log.h"

ActionHandler::ActionHandler() : keySender() {}

ActionHandler::~ActionHandler() {}

// returns a bool that tells the event handler 
// whether or not to block the event
bool ActionHandler::handleKeyEvent(int keyCode, int flags, std::string type) {
		Log::logToFile("action handler: Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));
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

