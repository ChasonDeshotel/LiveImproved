#pragma once

#include <functional>
#include <vector>
#include <memory>

#include "IPluginManager.h"
#include "Types.h"

class IIPCCore;
class ResponseParser;
class ILogHandler;
class Plugin;

class PluginManager : public IPluginManager {
public:
    PluginManager(
                  std::function<std::shared_ptr<ILogHandler>()> logHandler
                  , std::function<std::shared_ptr<IIPCCore>()> ipc
                  , std::function<std::shared_ptr<ResponseParser>()> responseParser
                  );

    ~PluginManager() override;

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;

    const std::vector<Plugin>& getPlugins() const override;
    void refreshPlugins() override;

private:
    std::function<std::shared_ptr<ILogHandler>()> log_;
    std::function<std::shared_ptr<IIPCCore>()> ipc_;
    std::function<std::shared_ptr<ResponseParser>()> responseParser_;
    std::vector<Plugin> plugins_;
};
