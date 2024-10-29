#include <functional>

#include "LogGlobal.h"

#include "IEventHandler.h"
#include "LiveInterface.h"
#include "PID.h"

LiveInterface::LiveInterface(std::function<std::shared_ptr<IEventHandler>()> eventHandler)
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

auto isAnyTextFieldFocused() -> bool {
    return false;
}
