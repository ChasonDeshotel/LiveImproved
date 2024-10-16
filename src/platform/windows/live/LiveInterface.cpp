#include <functional>

#include "LogGlobal.h"

#include "EventHandler.h"
#include "LiveInterface.h"
#include "PID.h"

LiveInterface::LiveInterface(std::function<std::shared_ptr<EventHandler>()> eventHandler)
    : ILiveInterface()
    , eventHandler_(std::move(eventHandler))
{}

LiveInterface::~LiveInterface() = default;

void setupPluginWindowChangeObserver(std::function<void()> callback) {
    return;
}

void removePluginWindowChangeObserver() {
    return;
}

void closeFocusedPlugin() {
    return;
}

void closeAllPlugins() {
    return;
}

void openAllPlugins() {
    return;
}

void tilePluginWindows() {
    return;
}
