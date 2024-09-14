#include "PluginManager.h"
#include "LogHandler.h"
#include "Types.h"

#include "ResponseParser.h"
#include "IPC.h"

PluginManager::PluginManager(IPC& ipc, ResponseParser& responseParser)
    : ipc_(ipc)
    , responseParser_(responseParser)
    , log_(LogHandler::getInstance()) {}

const std::vector<Plugin>& PluginManager::getPlugins() const {
    return plugins_;
}

void PluginManager::refreshPlugins() {
    ipc_.writeRequest("PLUGINS");

    // Set up a callback to handle the response asynchronously using dispatch
    ipc_.initReadWithEventLoop([this](const std::string& response) {
        try {
            if (!response.empty()) {
                //log_.debug("Received response: " + response);
                plugins_ = responseParser_.parsePlugins(response);
            } else {
                log_.error("Failed to receive a valid response.");
            }
        } catch (const std::exception &e) {
            throw std::runtime_error("Error fetching plugins: " + std::string(e.what()));
        }
    });
}
