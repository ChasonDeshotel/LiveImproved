#include <string>

#include "ApplicationManager.h"
#include "ActionHandler.h"
#include "LogHandler.h"
#include "PlatformDependent.h"
//#include "ActionHandler.h"

ApplicationManager::ApplicationManager()
    : logHandler_(LogHandler::getInstance()) {
}

void ApplicationManager::initialize() {
    LogHandler::getInstance().info("ApplicatonManager: init");
//    this->eventHandler_ = eventHandler;
//    this->ipcManager_ = ipcManager;
    eventHandler_ = new EventHandler(*this);
    eventHandler_->initialize();

    actionHandler_ = new ActionHandler(*this);
    keySender_ = new KeySender(*this);

    //actionHandler_ = &actionHandler;


    // Initialize platform-specific components
//    initializePlatform();

    LogHandler::getInstance().info("ApplicatonManager: init finished");
//
//    ipc.createPipe("/tmp/request_pipe");
//    ipc.createPipe("/tmp/response_pipe");
//    ipc.openPipeForWrite("/tmp/request_pipe", true);
//    ipc.openPipeForRead("/tmp/response_pipe", true);
}

//void ApplicationManager::run() {
//    runPlatform();
//}
//
//pid_t ApplicationManager::getAbletonLivePID() const {
//    return abletonLivePID;
//}
//
//void ApplicationManager::setAbletonLivePID(pid_t pid) {
//    abletonLivePID = pid;
//}
//
//void ApplicationManager::sendMessage(const std::string& message) {
//    ipc.writeToPipe("/tmp/request_pipe", message);
//}
//
//std::string ApplicationManager::receiveMessage() {
//    return ipc.readFromPipe("/tmp/response_pipe");
//}
//
ActionHandler* ApplicationManager::getActionHandler() {
    return actionHandler_;
}

EventHandler* ApplicationManager::getEventHandler() {
    return eventHandler_;
}

KeySender* ApplicationManager::getKeySender() {
    return keySender_;
}

//LogHandler& ApplicationManager::getLogHandler() {
//    return logHandler;
//}
