#pragma once

#include <vector>

class Plugin;

class IPluginManager {
public:
    virtual ~IPluginManager() = default;

    IPluginManager(const IPluginManager&) = delete;
    IPluginManager& operator=(const IPluginManager&) = delete;
    IPluginManager(IPluginManager&&) = delete;
    IPluginManager& operator=(IPluginManager&&) = delete;

    [[nodiscard]] virtual auto getPlugins() const -> const std::vector<Plugin>& = 0;
    virtual void refreshPlugins() = 0;

protected:
    IPluginManager() = default;
};
