#pragma once

// TODO cross-platform
#ifndef _WIN32
	#import <CoreGraphics/CoreGraphics.h>
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
class EventHandler;
class ILiveInterface;

class ActionHandler : public IActionHandler {
public:
  ActionHandler(std::function<std::shared_ptr<ILogHandler>()> logHandler,
                std::function<std::shared_ptr<IPluginManager>()> pluginManager,
                std::function<std::shared_ptr<WindowManager>()> windowManager,
                std::function<std::shared_ptr<ConfigManager>()> configManager,
                std::function<std::shared_ptr<IIPC>()> ipc,
                std::function<std::shared_ptr<EventHandler>()> eventHandler,
                std::function<std::shared_ptr<ILiveInterface>()> liveInterface);

  ~ActionHandler() override;

  ActionHandler(const ActionHandler &) = default;
  ActionHandler(ActionHandler &&) = delete;
  ActionHandler &operator=(const ActionHandler &) = default;
  ActionHandler &operator=(ActionHandler &&) = delete;

  void handleAction(std::string) override;

  // returns if the event should be blocking
  bool handleKeyEvent(EKeyPress pressedKey) override;

  void handleDoubleRightClick() override;

  bool loadItem(int itemIndex) override;
  bool loadItemByName(const std::string &itemName) override;

private:
    std::function<std::shared_ptr<IIPC>()> ipc_;
    std::function<std::shared_ptr<ILogHandler>()> logHandler_;
    std::function<std::shared_ptr<IPluginManager>()> pluginManager_;
    std::function<std::shared_ptr<ConfigManager>()> configManager_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;
    std::function<std::shared_ptr<EventHandler>()> eventHandler_;
    std::function<std::shared_ptr<ILiveInterface>()> liveInterface_;

    std::shared_ptr<ILogHandler> log() { return logHandler_(); }
    std::shared_ptr<WindowManager> windowManager() { return windowManager_(); }
    std::shared_ptr<ConfigManager> configManager() { return configManager_(); }
    std::shared_ptr<IIPC> ipc() { return ipc_(); }
    std::shared_ptr<IPluginManager> pluginManager() { return pluginManager_(); }
    void getMostRecentFloatingWindowDelayed(std::function<void(int)> callback);


    void initializeActionMap();
    void executeMacro(const EMacro& macro);

    bool closeWindows();

    //bool onEscapePress();
};
