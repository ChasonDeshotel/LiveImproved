#pragma once

#include "MockWindow.h"
#include "IWindowManager.h"
#include <map>

class MockWindowManager : public IWindowManager {
public:
    MockWindowManager() = default;
    ~MockWindowManager() override = default;

    MockWindowManager(const MockWindowManager &) = delete;
    MockWindowManager(MockWindowManager &&) = delete;
    MockWindowManager &operator=(const MockWindowManager &) = delete;
    MockWindowManager &operator=(MockWindowManager &&) = delete;

    auto registerWindow(const std::string& windowName, std::function<void()> callback) -> void override {
        windows_[windowName] = {.window = createWindowInstance(windowName), .callback = std::move(callback)};
        windowStates_[windowName] = false;
    }

    [[nodiscard]] auto getWindowHandle(const std::string& windowName) const -> void* override {
        auto it = windows_.find(windowName);
        if (it != windows_.end()) {
            return it->second.window->getWindowHandle();
        }
        throw std::runtime_error("Window not found for: " + windowName);
    }

    auto openWindow(const std::string& windowName) -> void override {
        auto it = windows_.find(windowName);
        if (it == windows_.end()) {
            registerWindow(windowName, []() {});
            it = windows_.find(windowName);
        }

        if (it->second.callback) {
            it->second.callback();
        }

        if (!windowStates_[windowName]) {
            windowStates_[windowName] = true;
            it->second.window->open();
        }
    }

    auto closeWindow(const std::string& windowName) -> void override {
        auto it = windows_.find(windowName);
        if (it != windows_.end() && windowStates_[windowName]) {
            it->second.window->close();
        }
        windowStates_[windowName] = false;
    }

    auto toggleWindow(const std::string& windowName) -> void override {
        auto it = windows_.find(windowName);
        if (it == windows_.end()) {
            openWindow(windowName);
            return;
        }

        bool isOpen = windowStates_[windowName];
        if (isOpen) {
            closeWindow(windowName);
        } else {
            openWindow(windowName);
        }
        windowStates_[windowName] = !isOpen;
    }

    [[nodiscard]] auto isWindowOpen(const std::string& windowName) const -> bool override {
        auto it = windowStates_.find(windowName);
        return (it != windowStates_.end() && it->second);
    }

    auto createWindowInstance(const std::string& windowName) -> std::unique_ptr<IWindow> override {
        return std::make_unique<MockWindow>(windowName);
    }

    // Helper methods for testing
    [[nodiscard]] auto getWindows() const -> const std::map<std::string, WindowData>& {
        return windows_;
    }

    [[nodiscard]] auto getWindowStates() const -> const std::map<std::string, bool>& {
        return windowStates_;
    }

private:
    std::map<std::string, WindowData> windows_;
    std::map<std::string, bool> windowStates_;
};
