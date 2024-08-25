#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include <string>

#include "LogHandler.h"

class ApplicationManager;

class ActionHandler {
public:
    ActionHandler(ApplicationManager& appManager);
    ~ActionHandler();

    void init();

    // returns if the event should be blocking
    bool handleKeyEvent(int keyCode, int flags, std::string type);

private:
    ApplicationManager& app_;
    LogHandler* log_;

    bool openSearchBox();
    bool closeSearchBox();

    bool loadItem();
    bool onEscapePress();
};

#endif
