#include "ContextMenu.h"
#include "ApplicationManager.h"
#include "LogHandler.h"
#include "ActionHandler.h"
#include "ConfigMenu.h"
#include "WindowManager.h"
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

ContextMenu::ContextMenu(std::function<void(const std::string&)> callback)
    : overrideCallback_(callback), isOpen_(false), hContextMenu_(NULL) {
    menuItems_ = ApplicationManager::getInstance().getConfigMenu()->getMenuData();
}

void* ContextMenu::getWindowHandle() const {
    return nullptr;
}

HMENU ContextMenu::createContextMenuWithItems(const std::vector<MenuItem>& items) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        LogHandler::getInstance().error("Failed to create context menu");
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
    if (isOpen_) return;  // Don't open if already open

    POINT mouseLocation;
    GetCursorPos(&mouseLocation);  // Get current mouse location

    if (!hContextMenu_) {
        hContextMenu_ = createContextMenuWithItems(menuItems_);
    }

    if (hContextMenu_) {
        isOpen_ = true;
        int selectedItem = TrackPopupMenu(hContextMenu_, TPM_RETURNCMD | TPM_NONOTIFY, mouseLocation.x, mouseLocation.y, 0, GetForegroundWindow(), NULL);

        if (selectedItem) {
            std::string selectedAction = reinterpret_cast<const char*>(selectedItem);
            if (overrideCallback_) {
                overrideCallback_(selectedAction);
            } else {
                ApplicationManager::getInstance().getActionHandler()->handleAction(selectedAction);
            }
        }
    }
}

void ContextMenu::close() {
    if (isOpen_) {
        LogHandler::getInstance().debug("ContextMenu::close called");
        if (hContextMenu_) {
            DestroyMenu(hContextMenu_);
            hContextMenu_ = NULL;
        }
        isOpen_ = false;
    }
}

bool ContextMenu::isOpen() const {
    return isOpen_;
}

void ContextMenu::setIsOpen(bool isOpen) {
    isOpen_ = isOpen;
}

void ContextMenu::closeMenu() {
    close();
}
