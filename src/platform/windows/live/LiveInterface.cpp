#include <functional>

#include "LogGlobal.h"

#include "IEventHandler.h"
#include "LiveInterface.h"
#include "ILiveInterface.h"
#include "PID.h"

LiveInterface::LiveInterface(std::function<std::shared_ptr<IEventHandler>()> eventHandler)
    : ILiveInterface()
    , eventHandler_(std::move(eventHandler))
{}

LiveInterface::~LiveInterface() = default;

void LiveInterface::setupPluginWindowChangeObserver(std::function<void()> callback) {
    return;
}

void LiveInterface::removePluginWindowChangeObserver() {
    return;
}

void LiveInterface::closeFocusedPlugin() {
    return;
}

void LiveInterface::closeAllPlugins() {
    return;
}

void LiveInterface::openAllPlugins() {
    return;
}

void LiveInterface::tilePluginWindows() {
    return;
}

auto LiveInterface::isAnyTextFieldFocused() -> bool {
    return false;
}
