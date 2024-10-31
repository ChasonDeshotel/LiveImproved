#pragma once

#include "ILiveInterface.h"
#include <functional>
#include <vector>

class MockLiveInterface : public ILiveInterface {
public:
    MockLiveInterface() = default;
    ~MockLiveInterface() override = default;

    void setupPluginWindowChangeObserver(std::function<void()> callback) override {
        m_callback = std::move(callback);
    }

    void removePluginWindowChangeObserver() override {
        m_callback = nullptr;
    }

    void closeFocusedPlugin() override {
        m_closeFocusedPluginCalled = true;
    }

    void closeAllPlugins() override {
        m_closeAllPluginsCalled = true;
    }

    void openAllPlugins() override {
        m_openAllPluginsCalled = true;
    }

    void tilePluginWindows() override {
        m_tilePluginWindowsCalled = true;
    }

    bool isAnyTextFieldFocused() override {
        return m_isAnyTextFieldFocused;
    }

    // Additional methods for controlling and inspecting the mock

    void triggerPluginWindowChange() {
        if (m_callback) {
            m_callback();
        }
    }

    bool wasCloseFocusedPluginCalled() const { return m_closeFocusedPluginCalled; }
    bool wasCloseAllPluginsCalled() const { return m_closeAllPluginsCalled; }
    bool wasOpenAllPluginsCalled() const { return m_openAllPluginsCalled; }
    bool wasTilePluginWindowsCalled() const { return m_tilePluginWindowsCalled; }

    void setIsAnyTextFieldFocused(bool value) { m_isAnyTextFieldFocused = value; }

    void reset() {
        m_callback = nullptr;
        m_closeFocusedPluginCalled = false;
        m_closeAllPluginsCalled = false;
        m_openAllPluginsCalled = false;
        m_tilePluginWindowsCalled = false;
        m_isAnyTextFieldFocused = false;
    }

private:
    std::function<void()> m_callback;
    bool m_closeFocusedPluginCalled = false;
    bool m_closeAllPluginsCalled = false;
    bool m_openAllPluginsCalled = false;
    bool m_tilePluginWindowsCalled = false;
    bool m_isAnyTextFieldFocused = false;
};
