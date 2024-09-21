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

    const std::vector<Plugin>& getPlugins() const override;
    void refreshPlugins() override;

private:
    std::function<std::shared_ptr<ILogHandler>()> log_;
    std::function<std::shared_ptr<IIPC>()> ipc_;
    std::function<std::shared_ptr<ResponseParser>()> responseParser_;
    std::vector<Plugin> plugins_;
};
