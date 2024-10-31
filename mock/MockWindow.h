#pragma once

#include "IWindow.h"
#include <string>

class MockWindow : public IWindow {
public:
    explicit MockWindow(const std::string& windowName) : name(windowName) {}
    ~MockWindow() override = default;

    MockWindow(const MockWindow &) = delete;
    MockWindow(MockWindow &&) = delete;
    MockWindow &operator=(const MockWindow &) = delete;
    MockWindow &operator=(MockWindow &&) = delete;

    void open() override { openCalled = true; }
    void close() override { closeCalled = true; }
    [[nodiscard]] void* getWindowHandle() const override { return windowHandle; }

    // Helper methods for testing
    [[nodiscard]] bool wasOpenCalled() const { return openCalled; }
    [[nodiscard]] bool wasCloseCalled() const { return closeCalled; }
    void setWindowHandle(void* handle) { windowHandle = handle; }
    [[nodiscard]] const std::string& getWindowName() const { return name; }

private:
    std::string name;
    bool openCalled = false;
    bool closeCalled = false;
    void* windowHandle = nullptr;
};
