#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

#include "IWindow.h"

class IActionHandler;
class IEventHandler;
class IPluginManager;
class IWindow;

class ConfigMenu;
class LimLookAndFeel;
class Theme;

class WindowManager {
public:
   WindowManager(
                 std::function<std::shared_ptr<IPluginManager>()> pluginManager
                 , std::function<std::shared_ptr<IEventHandler>()> eventHandler
                 , std::function<std::shared_ptr<IActionHandler>()> actionHandler
                 , std::function<std::shared_ptr<WindowManager>()> windowManager
                 , std::function<std::shared_ptr<Theme>()> theme
                 , std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel
                 , std::function<std::shared_ptr<ConfigMenu>()> configMenu
       );

    // TODO remove unused "override callback" param
    void registerWindow(const std::string& windowName, std::function<void()> callback = nullptr);
    [[nodiscard]] auto getWindowHandle(const std::string& windowName) const -> void*;

    void openWindow(const std::string& windowName);

    void closeWindow(const std::string& windowName);

    void toggleWindow(const std::string& windowName);

    // Check if a window is open
    [[nodiscard]] auto isWindowOpen(const std::string& windowName) const -> bool;

private:
    std::function<std::shared_ptr<IPluginManager>()> pluginManager_;
    std::function<std::shared_ptr<IEventHandler>()> eventHandler_;
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;
    std::function<std::shared_ptr<Theme>()> theme_;
    std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel_;
    std::function<std::shared_ptr<ConfigMenu>()> configMenu_;

    // Factory function to create window instances based on window name
    auto createWindowInstance(const std::string& windowName) -> std::unique_ptr<IWindow>;

    std::unordered_map<std::string, WindowData> windows_;
    std::unordered_map<std::string, bool> windowStates_;
 //   std::unordered_map<std::string, WindowFactory> windowFactories_;
//    std::unordered_map<std::string, struct { std::unique_ptr<IWindow> window; std::function<void()> callback; }> windows_;
};
