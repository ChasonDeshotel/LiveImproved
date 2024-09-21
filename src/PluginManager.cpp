#include <functional>
#include "PluginManager.h"
#include "ILogHandler.h"
#include "ResponseParser.h"
#include "IIPC.h"

#include "Types.h"

PluginManager::PluginManager(
                             std::function<std::shared_ptr<ILogHandler>()> logHandler
                             , std::function<std::shared_ptr<IIPC>()> ipc
                             , std::function<std::shared_ptr<ResponseParser>()> responseParser
    )
    : ipc_(std::move(ipc))
    , responseParser_(std::move(responseParser))
    , log_(std::move(logHandler)) {}

PluginManager::~PluginManager() {}

const std::vector<Plugin>& PluginManager::getPlugins() const {
    return plugins_;
}

void PluginManager::refreshPlugins() {
    auto ipc = ipc_();
    ipc->writeRequest("PLUGINS");

    // Set up a callback to handle the response asynchronously using dispatch
    ipc->initReadWithEventLoop([this](const std::string& response) {
        auto responseParser = responseParser_();
        auto log = log_();
        try {
            if (!response.empty()) {
                //log_.debug("Received response: " + response);
                plugins_ = responseParser->parsePlugins(response);
            } else {
                log->error("Failed to receive a valid response.");
            }
        } catch (const std::exception &e) {
            throw std::runtime_error("Error fetching plugins: " + std::string(e.what()));
        }
    });
}
