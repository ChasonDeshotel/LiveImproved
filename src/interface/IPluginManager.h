#pragma once

#include <memory>
#include <vector>
#include "IPluginManager.h"

class Plugin;

class IPluginManager {
public:
    virtual ~IPluginManager() = default;
    virtual const std::vector<Plugin>& getPlugins() const = 0;
    virtual void refreshPlugins() = 0;
};
