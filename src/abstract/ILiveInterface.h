#pragma once

#include <functional>

using WindowHandle = const void*;

class ILiveInterface {
public:
    virtual ~ILiveInterface() = default;

    ILiveInterface(const ILiveInterface &) = default;
    ILiveInterface(ILiveInterface &&) = delete;
    ILiveInterface &operator=(const ILiveInterface &) = default;
    ILiveInterface &operator=(ILiveInterface &&) = delete;

    virtual void setupPluginWindowChangeObserver(std::function<void()> callback) = 0;
    virtual void removePluginWindowChangeObserver() = 0;

    virtual void closeFocusedPlugin() = 0;
    virtual void closeAllPlugins() = 0;
    virtual void openAllPlugins() = 0;
    virtual void tilePluginWindows() = 0;

protected:
    ILiveInterface() = default;
};
