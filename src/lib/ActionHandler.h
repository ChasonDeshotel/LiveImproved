#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include <string>

#include "ApplicationManager.h"
#include "LogHandler.h"

class ApplicationManager;

class ActionHandler {
public:
    ActionHandler(ApplicationManager& appManager);
    ~ActionHandler();

    void init();

    // returns if the event should be blocking
    bool handleKeyEvent(CGKeyCode keyCode, CGEventFlags flags, std::string type);

    bool loadItem(int itemIndex);

private:
    ApplicationManager& app_;
    LogHandler* log_;

    bool openSearchBox();
    bool closeSearchBox();

    bool onEscapePress();
};

#endif
