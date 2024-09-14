#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include <CoreGraphics/CoreGraphics.h>
#include <string>

#include "LogHandler.h"
#include "Types.h"

class ApplicationManager;
class KeyMapper;
class PluginManager;
class IPC;
class WindowManager;
class ConfigManager;

class ActionHandler {
public:
    ActionHandler(IPC& ipc, PluginManager& pluginManager, WindowManager& windowManager, ConfigManager& configManager);
    ~ActionHandler();

    void init();

    void handleAction(std::string);
    // returns if the event should be blocking
    bool handleKeyEvent(std::string keyString, CGEventFlags flags, std::string type);
    
    void handleDoubleRightClick();

    bool loadItem(int itemIndex);
    bool loadItemByName(const std::string& itemName);

private:
    IPC& ipc_;
    LogHandler& log_;
    WindowManager& windowManager_;
    ConfigManager& configManager_;
    PluginManager& pluginManager_;

    void initializeActionMap();
    void executeMacro(const EMacro& macro);

    bool openSearchBox();
    bool closeWindows();

    bool onEscapePress();
};

#endif
