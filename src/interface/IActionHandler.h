#pragma once

#include <string>

class IPC;
class PluginManager;
class WindowManager;
class ConfigManager;
class EKeyPress;

class IActionHandler {
public:
    virtual ~IActionHandler() = default;

    IActionHandler(const IActionHandler &) = default;
    IActionHandler(IActionHandler &&) = delete;
    IActionHandler &operator=(const IActionHandler &) = default;
    IActionHandler &operator=(IActionHandler &&) = delete;

    virtual void handleAction(std::string action) = 0;
    virtual bool handleKeyEvent(EKeyPress pressedKey) = 0;
    virtual void handleDoubleRightClick() = 0;
    virtual bool loadItem(int itemIndex) = 0;
    virtual auto loadItemByName(const std::string &itemName) -> bool = 0;

protected:
    IActionHandler() = default;
};
