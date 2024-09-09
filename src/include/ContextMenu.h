#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include <vector>
#include <string>
#include <functional>

#include "Types.h"

class ContextMenu : public IWindow {
public:
    ContextMenu(std::function<void(const std::string&)> callback = nullptr);

    void open() override;
    void close() override;

    bool isOpen() const;
    void setIsOpen(bool isOpen);

private:
    std::vector<MenuItem> menuItems_;
    std::function<void(const std::string&)> actionCallback_;
    bool isOpen_ = false;
};

#endif
