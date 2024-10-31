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
        m_refreshCalled = true;
    }

    // Additional methods for controlling and inspecting the mock

    void setPlugins(const std::vector<Plugin>& plugins) {
        m_plugins = plugins;
    }

    bool wasRefreshCalled() const {
        return m_refreshCalled;
    }

    void reset() {
        m_plugins.clear();
        m_refreshCalled = false;
    }

private:
    std::vector<Plugin> m_plugins;
    bool m_refreshCalled = false;
};
