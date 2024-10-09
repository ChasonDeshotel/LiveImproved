#pragma once

#include <functional>
#include <vector>
#include <memory>

#include "IPluginManager.h"

class IIPC;
class ResponseParser;
class ILogHandler;
class Plugin;

class PluginManager : public IPluginManager {
public:
    PluginManager(
                  std::function<std::shared_ptr<ILogHandler>()> logHandler
                  , std::function<std::shared_ptr<IIPC>()> ipc
                  , std::function<std::shared_ptr<ResponseParser>()> responseParser
                  );

    ~PluginManager() override;

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;

    [[nodiscard]] auto getPlugins() const -> const std::vector<Plugin>& override;
    void refreshPlugins() override;

private:
    std::function<std::shared_ptr<ILogHandler>()> log_;
    std::function<std::shared_ptr<IIPC>()> ipc_;
    std::function<std::shared_ptr<ResponseParser>()> responseParser_;
    std::vector<Plugin> plugins_;
};
