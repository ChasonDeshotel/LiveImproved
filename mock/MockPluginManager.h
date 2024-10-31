#pragma once

#include "IPluginManager.h"
#include <vector>

class MockPluginManager : public IPluginManager {
public:
    MockPluginManager() = default;
    ~MockPluginManager() override = default;

    [[nodiscard]] auto getPlugins() const -> const std::vector<Plugin>& override {
        return m_plugins;
    }

    auto refreshPlugins() -> void override {
        // Implementation left empty intentionally
    }

private:
    std::vector<Plugin> m_plugins;
};
