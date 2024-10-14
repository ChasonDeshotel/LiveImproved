#pragma once

#include <vector>

#include "Types.h"

class IPluginManager {
public:
    virtual ~IPluginManager() = default;

    IPluginManager(const IPluginManager&) = delete;
    auto operator=(const IPluginManager&) -> IPluginManager& = delete;
    IPluginManager(IPluginManager&&) = delete;
    auto operator=(IPluginManager&&) -> IPluginManager& = delete;

    [[nodiscard]] virtual auto getPlugins() const -> const std::vector<Plugin>& = 0;
    virtual auto refreshPlugins() -> void = 0;

protected:
    IPluginManager() = default;
};
