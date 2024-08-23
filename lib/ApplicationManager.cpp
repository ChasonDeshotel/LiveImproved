#include <string>

#include "ApplicationManager.h"
#include "LogHandler.h"
#include "PlatformDependent.h"
#include "ActionHandler.h"

ApplicationManager::ApplicationManager()
    : logHandler_(&LogHandler::getInstance()) {
}

void ApplicationManager::initialize() {
    logHandler_->info("ApplicatonManager: init");

    eventHandler_ = new EventHandler(*this);
    eventHandler_->initialize(); // start event loop

    actionHandler_ = new ActionHandler(*this);
    keySender_ = new KeySender(*this);

    logHandler_->info("ApplicatonManager: init finished");
}

ActionHandler* ApplicationManager::getActionHandler() {
    return actionHandler_;
}

EventHandler* ApplicationManager::getEventHandler() {
    return eventHandler_;
}

KeySender* ApplicationManager::getKeySender() {
    return keySender_;
}

LogHandler* ApplicationManager::getLogHandler() {
    return logHandler_;
}
