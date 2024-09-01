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
    bool handleKeyEvent(std::string keyString, CGEventFlags flags, std::string type);
    
    void handleDoubleRightClick();

    bool loadItem(int itemIndex);

private:
    ApplicationManager& app_;
    LogHandler* log_;

    void initializeActionMap();
    void foobar();
    void sendKeypress(const std::string& key);

    bool openSearchBox();
    bool closeSearchBox();

    bool onEscapePress();
};

#endif
