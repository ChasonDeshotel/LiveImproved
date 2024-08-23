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

    // crashed when chaining
    eventHandler_ = new EventHandler(*this);
    eventHandler_->init(); // start event loop

    ipc_ = new IPC(*this);
    ipc_->init();

    actionHandler_ = new ActionHandler(*this);
    keySender_ = new KeySender(*this);

    guiSearchBox_ = new GUISearchBox(*this);
    std::vector<std::string> options = {"foo", "bar", "Option 1", "Option 2", "Option 3"};
    guiSearchBox_->setOptions(options);

    logHandler_->info("ApplicatonManager::init() finished");
}

LogHandler* ApplicationManager::getLogHandler() {
    return logHandler_;
}

pid_t ApplicationManager::getPID() {
    return pid_->livePID();
}

IPC* ApplicationManager::getIPC() {
    return ipc_;
}

EventHandler* ApplicationManager::getEventHandler() {
    return eventHandler_;
}

ActionHandler* ApplicationManager::getActionHandler() {
    return actionHandler_;
}

KeySender* ApplicationManager::getKeySender() {
    return keySender_;
}

GUISearchBox* ApplicationManager::getGUISearchBox() {
    return guiSearchBox_;
}

