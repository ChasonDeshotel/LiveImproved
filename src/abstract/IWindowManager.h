#pragma once

#include <string>
#include <functional>
#include <memory>

class IWindow;

class IWindowManager {
public:
    virtual ~IWindowManager() = default;

    IWindowManager(const IWindowManager &) = default;
    IWindowManager(IWindowManager &&) = delete;
    auto operator=(const IWindowManager &) -> IWindowManager & = default;
    auto operator=(IWindowManager &&) -> IWindowManager & = delete;

    virtual auto registerWindow(const std::string &windowName,
                              std::function<void()> callback) -> void = 0;
    [[nodiscard]] virtual auto getWindowHandle(const std::string &windowName) const -> void * = 0;
    virtual auto openWindow(const std::string &windowName) -> void = 0;
    virtual auto closeWindow(const std::string &windowName) -> void = 0;
    virtual auto toggleWindow(const std::string &windowName) -> void = 0;
    [[nodiscard]] virtual auto isWindowOpen(const std::string &windowName) const -> bool = 0;

protected:
    IWindowManager() = default;

    virtual auto createWindowInstance(const std::string& windowName) -> std::unique_ptr<IWindow> = 0;
};
