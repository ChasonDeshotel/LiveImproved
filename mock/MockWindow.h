#pragma once

#include "IWindow.h"
#include <vector>
#include <string>

class MockWindow : public IWindow {
public:
    MockWindow() = default;
    ~MockWindow() override = default;

    void open() override { openCalled = true; }
    void close() override { closeCalled = true; }
    [[nodiscard]] void* getWindowHandle() const override { return windowHandle; }

    // Helper methods for testing
    [[nodiscard]] bool wasOpenCalled() const { return openCalled; }
    [[nodiscard]] bool wasCloseCalled() const { return closeCalled; }
    void setWindowHandle(void* handle) { windowHandle = handle; }

private:
    bool openCalled = false;
    bool closeCalled = false;
    void* windowHandle = nullptr;
};
