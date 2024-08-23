#include "ActionHandler.h"
#include "ApplicationManager.h"

ActionHandler::ActionHandler(ApplicationManager& appManager) :
    app_(appManager)
{}

ActionHandler::~ActionHandler() {}

void ActionHandler::Init() {
    // should do the mapping / read config or something
}

// returns a bool that tells the event handler 
// whether or not to block the event
bool ActionHandler::handleKeyEvent(int keyCode, int flags, std::string type) {
    app_.getLogHandler()->info("action handler: Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));
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
    app_.getLogHandler()->info("Escape pressed");
    return true;
}

