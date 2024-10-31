#pragma once

#include "MockWindow.h"
#include "IWindowManager.h"
#include <map>
#include <set>

class MockWindowManager : public IWindowManager {
public:
    MockWindowManager() = default;
    ~MockWindowManager() override = default;

    MockWindowManager(const MockWindowManager &) = default;
    MockWindowManager(MockWindowManager &&) = delete;
    MockWindowManager &operator=(const MockWindowManager &) = default;
    MockWindowManager &operator=(MockWindowManager &&) = delete;

    auto registerWindow(const std::string& windowName, std::function<void()> callback) -> void override {
        registeredWindows_[windowName] = std::move(callback);
    }

    [[nodiscard]] auto getWindowHandle(const std::string& windowName) const -> void* override {
        return const_cast<void*>(static_cast<const void*>(windowName.c_str()));
    }

    auto openWindow(const std::string& windowName) -> void override {
        openWindows_.insert(windowName);
    }

    auto closeWindow(const std::string& windowName) -> void override {
        openWindows_.erase(windowName);
    }

    auto toggleWindow(const std::string& windowName) -> void override {
        if (isWindowOpen(windowName)) {
            closeWindow(windowName);
        } else {
            openWindow(windowName);
        }
    }

    [[nodiscard]] auto isWindowOpen(const std::string& windowName) const -> bool override {
        return openWindows_.find(windowName) != openWindows_.end();
    }

    auto createWindowInstance(const std::string& windowName) -> std::unique_ptr<IWindow> override {
        return std::make_unique<MockWindow>(windowName);
    }

    // Helper methods for testing
    [[nodiscard]] auto getRegisteredWindows() const -> const std::map<std::string, std::function<void()>>& {
        return registeredWindows_;
    }

    [[nodiscard]] auto getOpenWindows() const -> const std::set<std::string>& {
        return openWindows_;
    }

private:
    std::map<std::string, std::function<void()>> registeredWindows_;
    std::set<std::string> openWindows_;

};
