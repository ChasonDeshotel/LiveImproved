#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

class IWindow {
public:
    virtual ~IWindow() = default;

    IWindow(const IWindow&) = delete;
    auto operator=(const IWindow&) -> IWindow& = delete;
    IWindow(IWindow&&) = delete;
    auto operator=(IWindow&&) -> IWindow& = delete;

    virtual void open() = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual auto getWindowHandle() const -> void* = 0;

protected:
    IWindow() = default;
};

enum class Window {
    ContextMenu
};

struct WindowData {
    std::shared_ptr<IWindow> window;
    std::function<void()> callback;
};

struct MenuItem {
    std::string label;
    std::string action;
    std::vector<MenuItem> children;
};

