#pragma once

#include <functional>
#include <map>
#include <set>
#include <vector>

#include "ILiveInterface.h"

class EventHandler;

class LiveInterface : public ILiveInterface {
public:
    LiveInterface(
        std::function<std::shared_ptr<EventHandler>()> eventHandler
    );

    ~LiveInterface() override;

    void setupPluginWindowChangeObserver(std::function<void()> callback) override;
    void removePluginWindowChangeObserver() override;

    void closeFocusedPlugin() override;
    void closeAllPlugins() override;
    void openAllPlugins() override;
    void tilePluginWindows() override;

private:
    std::function<std::shared_ptr<EventHandler>()> eventHandler_;

};
