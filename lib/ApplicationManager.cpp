#include <string>

#include "ApplicationManager.h"
#include "LogHandler.h"
#include "PlatformDependent.h"

void ApplicationManager::initialize(EventHandler& eventHandler, KeySender& keySender) {
    LogHandler::getInstance().info("ApplicatonManager: init");
//    this->eventHandler_ = eventHandler;
//    this->ipcManager_ = ipcManager;
    keySender_ = &keySender;  // Store the pointer to the reference
    eventHandler_ = &eventHandler;

    eventHandler.initialize();
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
//ActionHandler& ApplicationManager::getActionHandler() {
//    return actionHandler;
//}
//
EventHandler& ApplicationManager::getEventHandler() {
    return *eventHandler_;
}

KeySender& ApplicationManager::getKeySender() {
    return *keySender_;
}

//LogHandler& ApplicationManager::getLogHandler() {
//    return logHandler;
//}
