#include <functional>
#include "PluginManager.h"
#include "ILogHandler.h"
#include "ResponseParser.h"
#include "IIPCCore.h"

#include "Types.h"

PluginManager::PluginManager(
                             std::function<std::shared_ptr<ILogHandler>()> logHandler
                             , std::function<std::shared_ptr<IIPCCore>()> ipc
                             , std::function<std::shared_ptr<ResponseParser>()> responseParser
    )
    : ipc_(std::move(ipc))
    , responseParser_(std::move(responseParser))
    , log_(std::move(logHandler)) {}

PluginManager::~PluginManager() = default;

const std::vector<Plugin>& PluginManager::getPlugins() const {
    return plugins_;
}

void PluginManager::refreshPlugins() {
    auto ipc = ipc_();
    ipc->writeRequest("PLUGINS", [this](const std::string& response) {
        auto responseParser = responseParser_();
        auto log = log_();
        try {
            if (!response.empty()) {
                plugins_ = responseParser->parsePlugins(response);
                log->info("Plugin cache refreshed");
            } else {
                log->error("Failed to receive a valid response");
            }
        } catch (const std::exception &e) {
            throw std::runtime_error("Error fetching plugins: " + std::string(e.what()));
        }
    });
}
