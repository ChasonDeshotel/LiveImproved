#pragma once

#include <string>

class EKeyPress;

class IActionHandler {
public:
    virtual ~IActionHandler() = default;

    IActionHandler(const IActionHandler &) = default;
    IActionHandler(IActionHandler &&) = delete;
    auto operator=(const IActionHandler &) -> IActionHandler & = default;
    auto operator=(IActionHandler &&) -> IActionHandler & = delete;

    virtual auto handleAction(std::string action) -> void = 0;
    virtual auto handleKeyEvent(EKeyPress pressedKey) -> bool = 0;
    virtual auto handleDoubleRightClick() -> void = 0;
    virtual auto loadItem(int itemIndex) -> bool = 0;
    virtual auto loadItemByName(const std::string &itemName) -> bool = 0;

protected:
    IActionHandler() = default;
};
