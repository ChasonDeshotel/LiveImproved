#pragma once

#include <vector>
#include <string>
#include <functional>

#include "IWindow.h"

#ifdef __OBJC__
@class ContextMenuGenerator;
@class NSMenu;
#else
class ContextMenuGenerator;
class NSMenu;
#endif

class ILogHandler;
class ConfigMenu;
class IActionHandler;
class WindowManager;

class ContextMenu : public IWindow {
public:
    ContextMenu(
        std::function<std::shared_ptr<ILogHandler>()> log
        , std::function<std::shared_ptr<ConfigMenu>()> configMenu
        , std::function<std::shared_ptr<IActionHandler>()> actionHandler
        , std::function<std::shared_ptr<WindowManager>()> windowManager
    );
    ~ContextMenu(); // Define a destructor to clean up the Objective-C object

    void* getWindowHandle() const override;
    void open() override;
    void close() override;

    bool isOpen() const;
    void setIsOpen(bool isOpen);

    void closeMenu();

private:
    std::function<std::shared_ptr<ILogHandler>()> log_;
    std::function<std::shared_ptr<ConfigMenu>()> configMenu_;
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;

    std::vector<MenuItem> menuItems_;
    bool isOpen_ = false;

    NSMenu* contextMenu_;
    ContextMenuGenerator* menuGenerator_;
    void generateMenu();
};
