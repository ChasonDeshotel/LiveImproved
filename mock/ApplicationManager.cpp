#include "ApplicationManager.h"
#include "LogHandler.h"

ApplicationManager::ApplicationManager() 
    : mockLogHandler(LogHandler::getInstance()) {}

LogHandler* ApplicationManager::getLogHandler() {
    return &mockLogHandler;
}
