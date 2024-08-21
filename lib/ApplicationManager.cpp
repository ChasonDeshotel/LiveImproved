#include "ApplicationManager.h"
#include "PlatformSpecific.h"  // Interface for platform-specific functions

ApplicationManager::ApplicationManager() {
    // Initialize IPC or other cross-platform setups
}

void ApplicationManager::initialize() {
    // Initialize platform-specific components
    initializePlatform();

    ipc.createPipe("/tmp/request_pipe");
    ipc.createPipe("/tmp/response_pipe");
    ipc.openPipeForWrite("/tmp/request_pipe", true);
    ipc.openPipeForRead("/tmp/response_pipe", true);
}

void ApplicationManager::run() {
    // Run the main event loop or equivalent cross-platform code
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
