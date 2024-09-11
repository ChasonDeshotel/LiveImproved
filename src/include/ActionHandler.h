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

    void handleAction(std::string);
    // returns if the event should be blocking
    bool handleKeyEvent(std::string keyString, CGEventFlags flags, std::string type);
    
    void handleDoubleRightClick();

    bool loadItem(int itemIndex);
    bool loadItemByName(const std::string& itemName);

private:
    ApplicationManager& app_;
    LogHandler* log_;

    void initializeActionMap();
    void sendKeypress(const std::string& key);

    bool openSearchBox();
    bool closeWindows();

    bool onEscapePress();
};

#endif
