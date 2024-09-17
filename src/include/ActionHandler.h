#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

// TODO cross-platform
#ifndef _WIN32
	#include <CoreGraphics/CoreGraphics.h>
#endif

#include <string>

#include "Types.h"

class LogHandler;
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
    // TODO cross-platform send EKeyPress
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

    bool closeWindows();

    //bool onEscapePress();
};

#endif
