#include <string>

#include "ActionHandler.h"
#include "ApplicationManager.h"
#include "LogHandler.h"
#include "PlatformSpecific.h"

ApplicationManager::ApplicationManager()
    : actionHandler()
      , eventHandler() 
{}

void ApplicationManager::initialize() {
    // Initialize platform-specific components
    initializePlatform();

    LogHandler::getInstance().info("ApplicatonManager: init");

    ipc.createPipe("/tmp/request_pipe");
    ipc.createPipe("/tmp/response_pipe");
    ipc.openPipeForWrite("/tmp/request_pipe", true);
    ipc.openPipeForRead("/tmp/response_pipe", true);
}

void ApplicationManager::run() {
    runPlatform();
}

pid_t ApplicationManager::getAbletonLivePID() const {
    return abletonLivePID;
}

void ApplicationManager::setAbletonLivePID(pid_t pid) {
    abletonLivePID = pid;
}

void ApplicationManager::sendMessage(const std::string& message) {
    ipc.writeToPipe("/tmp/request_pipe", message);
}

std::string ApplicationManager::receiveMessage() {
    return ipc.readFromPipe("/tmp/response_pipe");
}

ActionHandler& ApplicationManager::getActionHandler() {
    return actionHandler;
}

EventHandler& ApplicationManager::getEventHandler() {
    return eventHandler;
}

//LogHandler& ApplicationManager::getLogHandler() {
//    return logHandler;
//}
