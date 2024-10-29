#pragma once

#include <functional>
#include <map>
#include <set>
#include <vector>

#include "ILiveInterface.h"

class IEventHandler;

class LiveInterface : public ILiveInterface {
public:
    LiveInterface(
        std::function<std::shared_ptr<IEventHandler>()> eventHandler
    );

    ~LiveInterface() override;

    LiveInterface(const LiveInterface &) = default;
    LiveInterface(LiveInterface &&) = delete;
    LiveInterface &operator=(const LiveInterface &) = default;
    LiveInterface &operator=(LiveInterface &&) = delete;

    void setupPluginWindowChangeObserver(std::function<void()> callback) override;
    void removePluginWindowChangeObserver() override;

    void closeFocusedPlugin() override;
    void closeAllPlugins() override;
    void openAllPlugins() override;
    void tilePluginWindows() override;

private:
    std::function<std::shared_ptr<IEventHandler>()> eventHandler_;

};
