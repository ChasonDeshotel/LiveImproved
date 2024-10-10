#pragma once

#include <vector>

#include "Types.h"

class IPluginManager {
public:
    virtual ~IPluginManager() = default;

    IPluginManager(const IPluginManager&) = delete;
    IPluginManager& operator=(const IPluginManager&) = delete;
    IPluginManager(IPluginManager&&) = delete;
    IPluginManager& operator=(IPluginManager&&) = delete;

    virtual const std::vector<Plugin>& getPlugins() const = 0;
    virtual void refreshPlugins() = 0;

protected:
    IPluginManager() = default;
};
