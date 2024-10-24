#pragma once

#include <functional>
#include <vector>
#include <memory>

#include "IPluginManager.h"
#include "Types.h"

class IIPCQueue;
class ResponseParser;
class Plugin;

class PluginManager : public IPluginManager {
public:
    PluginManager(
                  std::function<std::shared_ptr<IIPCQueue>()> ipc
                  , std::function<std::shared_ptr<ResponseParser>()> responseParser
                  );

    ~PluginManager() override;

    PluginManager(const PluginManager&) = delete;
    auto operator=(const PluginManager&) -> PluginManager& = delete;
    PluginManager(PluginManager&&) = delete;
    auto operator=(PluginManager&&) -> PluginManager& = delete;

    [[nodiscard]] auto getPlugins() const -> const std::vector<Plugin>& override;
    void refreshPlugins() override;

private:
    std::function<std::shared_ptr<IIPCQueue>()> ipc_;
    std::function<std::shared_ptr<ResponseParser>()> responseParser_;
    std::vector<Plugin> plugins_;
};
