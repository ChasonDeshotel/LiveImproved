#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include <vector>
#include <string>
#include <functional>

#include "Types.h"

#ifdef __OBJC__
@class ContextMenuGenerator;
#endif

class ContextMenu : public IWindow {
public:
    ContextMenu(std::function<void(const std::string&)> callback = nullptr);

    void open() override;
    void close() override;

    bool isOpen() const;
    void setIsOpen(bool isOpen);

    void closeMenu();

private:
    std::vector<MenuItem> menuItems_;
    std::function<void(const std::string&)> actionCallback_;
    bool isOpen_ = false;

    #ifdef __OBJC__
        ContextMenuGenerator* menuGenerator_;
    #endif
};

#endif
