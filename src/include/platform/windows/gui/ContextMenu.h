#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

#include "IWindow.h"
#include "Types.h"

class ConfigMenu;
class IActionHandler;
class WindowManager;

class ContextMenu : public IWindow {
public:
    ContextMenu(std::function<std::shared_ptr<ConfigMenu>()> configMenu,
                std::function<std::shared_ptr<IActionHandler>()> actionHandler,
                std::function<std::shared_ptr<WindowManager>()> windowManager);
    ~ContextMenu() override;

    [[nodiscard]] void* getWindowHandle() const;

    void open();
    void close();
    [[nodiscard]] bool isOpen() const;

private:
    std::function<std::shared_ptr<ConfigMenu>()> configMenu_;
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;

    std::vector<MenuItem> menuItems_;                         // Holds menu items
    HMENU createContextMenuWithItems(const std::vector<MenuItem>& items);
    HMENU hContextMenu_;                                       // Windows handle for the context menu
};
