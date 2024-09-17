#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include "Types.h"

class ContextMenu {
public:
    // Constructor
    ContextMenu(std::function<void(const std::string&)> callback = nullptr);

    // Window handle related methods
    void* getWindowHandle() const;

    // Menu control methods
    void open();
    void close();
    bool isOpen() const;
    void setIsOpen(bool isOpen);

    // Utility method for closing the menu
    void closeMenu();

private:
    // Private method to recursively create a context menu
    HMENU createContextMenuWithItems(const std::vector<MenuItem>& items);

    // Private member variables
    std::vector<MenuItem> menuItems_;                         // Holds menu items
    std::function<void(const std::string&)> overrideCallback_; // Callback function for handling actions
    bool isOpen_;                                              // Flag indicating if the menu is open
    HMENU hContextMenu_;                                       // Windows handle for the context menu
};

#endif // CONTEXT_MENU_H