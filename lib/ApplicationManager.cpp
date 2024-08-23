#include <string>

#include "ApplicationManager.h"
#include "LogHandler.h"
#include "PlatformDependent.h"
#include "ActionHandler.h"

ApplicationManager::ApplicationManager()
    : logHandler_(&LogHandler::getInstance()) {
}

void ApplicationManager::init() {
    logHandler_->info("ApplicatonManager::init() called");

    pid_ = (new PID(*this))->init();

    ipc_ = new IPC(*this);

    // crashed when chaining
    eventHandler_ = new EventHandler(*this);
    eventHandler_->init(); // start event loop

    actionHandler_ = new ActionHandler(*this);
    keySender_ = new KeySender(*this);

    logHandler_->info("ApplicatonManager::init() finished");
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

pid_t ApplicationManager::getPID() {
    return pid_->livePID();
}
