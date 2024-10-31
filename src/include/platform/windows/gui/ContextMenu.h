#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

#include "IWindow.h"
#include "IWindowManager.h"
#include "Types.h"

class ConfigMenu;
class IActionHandler;

class ContextMenu : public IWindow {
public:
    ContextMenu(std::function<std::shared_ptr<ConfigMenu>()> configMenu,
                std::function<std::shared_ptr<IActionHandler>()> actionHandler,
                std::function<std::shared_ptr<IWindowManager>()> windowManager);
    ~ContextMenu() override;

    ContextMenu(const ContextMenu &) = delete;
    ContextMenu(ContextMenu &&) = delete;
    auto operator=(const ContextMenu &) -> ContextMenu & = delete;
    auto operator=(ContextMenu &&) -> ContextMenu & = delete;

    [[nodiscard]] void* getWindowHandle() const override;

    void open() override;
    void close() override;
    //[[nodiscard]] bool isOpen() const override;

private:
    std::function<std::shared_ptr<ConfigMenu>()> configMenu_;
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<IWindowManager>()> windowManager_;

    std::vector<MenuItem> menuItems_;                         // Holds menu items
    HMENU createContextMenuWithItems(const std::vector<MenuItem>& items);
    HMENU hContextMenu_;                                       // Windows handle for the context menu
};
