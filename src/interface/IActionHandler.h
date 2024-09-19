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

    virtual void init() = 0;
    virtual void handleAction(std::string) = 0;
    virtual bool handleKeyEvent(EKeyPress pressedKey) = 0;
    virtual void handleDoubleRightClick() = 0;
    virtual bool loadItem(int itemIndex) = 0;
    virtual bool loadItemByName(const std::string& itemName) = 0;

protected:
    IActionHandler() = default;
};
