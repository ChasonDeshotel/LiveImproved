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

std::vector<std::string> ApplicationManager::splitStringInPlace(std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    char* start = &str[0];  // Pointer to the beginning of the string
    char* end = start;

    while (*end != '\0') {  // Loop until the end of the string
        if (*end == delimiter) {
            *end = '\0';  // Replace the delimiter with a null terminator
            tokens.push_back(start);  // Store the start pointer as a string in the vector
            start = end + 1;  // Move the start pointer to the next character
        }
        end++;
    }

    tokens.push_back(start);  // Add the last token after the loop ends

    return tokens;
}

std::string ApplicationManager::getPluginCacheAsStr() const {
    if (pluginCache.empty()) {
        return "";
    }

    return std::accumulate(std::next(pluginCache.begin()), pluginCache.end(), pluginCache[0],
        [](const std::string& a, const std::string& b) { return a + ',' + b; });
}

void ApplicationManager::refreshPluginCache() {
    ipc_->writeRequest("PLUGINS");
    // Set up a callback to handle the response asynchronously using dispatch
    ipc_->initReadWithEventLoop([this](const std::string& response) {
        if (!response.empty()) {
        LogHandler::getInstance().info("Received response: " + response);
            std::string mutableResponse = response;
            pluginCache = splitStringInPlace(mutableResponse, ',');
        } else {
            LogHandler::getInstance().info("Failed to receive a valid response.");
        }
    });
}
