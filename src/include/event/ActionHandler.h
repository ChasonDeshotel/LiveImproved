#pragma once

// TODO cross-platform
#ifndef _WIN32
#import <CoreGraphics/CoreGraphics.h>
#endif

#include <string>

#include "Types.h"
#include "IActionHandler.h"

class IEventHandler;
class IIPC;
class ILiveInterface;
class IPluginManager;

class ConfigManager;
class KeyMapper;
class ResponseParser;
class WindowManager;

class ActionHandler : public IActionHandler {
public:
  ActionHandler(std::function<std::shared_ptr<IPluginManager>()>   pluginManager
                , std::function<std::shared_ptr<WindowManager>()>  windowManager
                , std::function<std::shared_ptr<ConfigManager>()>  configManager
                , std::function<std::shared_ptr<IIPC>()>       ipc
                , std::function<std::shared_ptr<IEventHandler>()>   eventHandler
                , std::function<std::shared_ptr<ILiveInterface>()> liveInterface
    );

    ~ActionHandler() override;

    ActionHandler(const ActionHandler &) = default;
    ActionHandler(ActionHandler &&) = delete;
    auto operator=(const ActionHandler &) -> ActionHandler & = default;
    auto operator=(ActionHandler &&) -> ActionHandler & = delete;

    void handleAction(std::string) override;

    // returns if the event should be blocking
    auto handleKeyEvent(EKeyPress pressedKey) -> bool override;

    void handleDoubleRightClick() override;

    auto loadItem(int itemIndex) -> bool override;
    auto loadItemByName(const std::string &itemName) -> bool override;

private:
    std::function<std::shared_ptr<IIPC>()> ipc_;
    std::function<std::shared_ptr<IPluginManager>()> pluginManager_;
    std::function<std::shared_ptr<ConfigManager>()> configManager_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;
    std::function<std::shared_ptr<IEventHandler>()> eventHandler_;
    std::function<std::shared_ptr<ILiveInterface>()> liveInterface_;

    using ActionHandlerFunction = std::function<void(const std::optional<std::string>& args)>;
    std::unordered_map<std::string, ActionHandlerFunction> actionMap;

    void getMostRecentFloatingWindowDelayed(std::function<void(int)> callback);

    void initializeActionMap();
    void executeMacro(const EMacro& macro);

    auto closeWindows() -> bool;
};
