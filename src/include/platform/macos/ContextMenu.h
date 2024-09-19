#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include <vector>
#include <string>
#include <functional>

#include "IWindow.h"

#ifdef __OBJC__
@class ContextMenuGenerator;
#endif
class ILogHandler;
class ConfigMenu;
class ActionHandler;
class WindowManager;

class ContextMenu : public IWindow {
public:
    ContextMenu(
        std::shared_ptr<ILogHandler> log
        , std::shared_ptr<ConfigMenu> configMenu 
        , std::shared_ptr<ActionHandler> actionHandler
        , std::shared_ptr<WindowManager> windowManager
    );

    void* getWindowHandle() const override;
    void open() override;
    void close() override;

    bool isOpen() const;
    void setIsOpen(bool isOpen);

    void closeMenu();

private:
    std::shared_ptr<ILogHandler> log_;
    std::shared_ptr<ConfigMenu> configMenu_;
    std::shared_ptr<ActionHandler> actionHandler_;
    std::shared_ptr<WindowManager> windowManager_;

    std::vector<MenuItem> menuItems_;
    bool isOpen_ = false;

    #ifdef __OBJC__
        ContextMenuGenerator* menuGenerator_;
    #endif
};

#endif
