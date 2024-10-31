#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "LogGlobal.h"

#include "IActionHandler.h"
#include "IWindow.h"
#include "IWindowManager.h"

#include "ContextMenu.h"
#include "ConfigMenu.h"

#include <string>
#include <vector>
#include <functional>

ContextMenu::ContextMenu(
        std::function<std::shared_ptr<ConfigMenu>()> configMenu
        , std::function<std::shared_ptr<IActionHandler>()> actionHandler
        , std::function<std::shared_ptr<IWindowManager>()> windowManager
    )
    : IWindow()
    , configMenu_(std::move(configMenu))
    , actionHandler_(std::move(actionHandler))
    , windowManager_(std::move(windowManager))
    , hContextMenu_()
    {}
    // TODO
    //menuItems_ = getConfigMenu()->getMenuData();
    //, menuItems_(configMenu_()->getMenuData())
    //, menuGenerator_(nil)

ContextMenu::~ContextMenu() = default;

void* ContextMenu::getWindowHandle() const {
    return nullptr;
}

HMENU ContextMenu::createContextMenuWithItems(const std::vector<MenuItem>& items) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        logger->error("Failed to create context menu");
        return NULL;
    }

    for (const auto& item : items) {
        if (item.children.empty()) {
            AppendMenu(hMenu, MF_STRING, reinterpret_cast<UINT_PTR>(item.action.c_str()), item.label.c_str());
        } else {
            // Create a submenu
            HMENU hSubMenu = createContextMenuWithItems(item.children);
            AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), item.label.c_str());
        }
    }

    return hMenu;
}

void ContextMenu::open() {
    POINT mouseLocation;
    GetCursorPos(&mouseLocation);

    if (!hContextMenu_) {
        hContextMenu_ = createContextMenuWithItems(menuItems_);
    }

    if (hContextMenu_) {
        int selectedItem = TrackPopupMenu(hContextMenu_, TPM_RETURNCMD | TPM_NONOTIFY, mouseLocation.x, mouseLocation.y, 0, GetForegroundWindow(), NULL);

        if (selectedItem) {
            std::string selectedAction = reinterpret_cast<const char*>(selectedItem);
            //actionHandler_()->handleAction(selectedAction);
        }
    }
}

void ContextMenu::close() {
    logger->debug("ContextMenu::close called");
    if (hContextMenu_) {
        DestroyMenu(hContextMenu_);
        hContextMenu_ = NULL;
    }
}
