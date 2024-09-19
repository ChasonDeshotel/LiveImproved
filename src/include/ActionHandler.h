#pragma once

// TODO cross-platform
#ifndef _WIN32
	#include <CoreGraphics/CoreGraphics.h>
#endif

#include <string>

#include "IActionHandler.h"
#include "Types.h"

class ILogHandler;
class KeyMapper;
class IPluginManager;
class ResponseParser;
class IIPC;
class WindowManager;
class ConfigManager;

class ActionHandler : public IActionHandler {
public:
    ActionHandler(
                  std::shared_ptr<ILogHandler> logHandler
                  , std::shared_ptr<IPluginManager> pluginManager
                  , std::shared_ptr<WindowManager> windowManager
                  , std::shared_ptr<ConfigManager> configManager
                  , std::shared_ptr<IIPC> ipc
                  );

    ~ActionHandler();

    void init();

    void handleAction(std::string);

    // returns if the event should be blocking
    bool handleKeyEvent(EKeyPress pressedKey);
    
    void handleDoubleRightClick();

    bool loadItem(int itemIndex);
    bool loadItemByName(const std::string& itemName);

private:
    std::shared_ptr<IIPC> ipc_;
    std::shared_ptr<ILogHandler> log_;
    std::shared_ptr<IPluginManager> pluginManager_;
    std::shared_ptr<ConfigManager> configManager_;
    std::shared_ptr<WindowManager> windowManager_;

    void initializeActionMap();
    void executeMacro(const EMacro& macro);

    bool closeWindows();

    //bool onEscapePress();
};
