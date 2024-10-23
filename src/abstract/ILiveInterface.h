#pragma once

#include <functional>

using WindowHandle = const void*;

class ILiveInterface {
public:
    virtual ~ILiveInterface() = default;

    ILiveInterface(const ILiveInterface &) = default;
    ILiveInterface(ILiveInterface &&) = delete;
    auto operator=(const ILiveInterface &) -> ILiveInterface & = default;
    auto operator=(ILiveInterface &&) -> ILiveInterface & = delete;

    virtual auto setupPluginWindowChangeObserver(std::function<void()> callback) -> void = 0;
    virtual auto removePluginWindowChangeObserver() -> void = 0;

    virtual auto closeFocusedPlugin() -> void = 0;
    virtual auto closeAllPlugins() -> void = 0;
    virtual auto openAllPlugins() -> void = 0;
    virtual auto tilePluginWindows() -> void = 0;

    virtual auto isAnyTextFieldFocused() -> bool = 0;

protected:
    ILiveInterface() = default;
};
