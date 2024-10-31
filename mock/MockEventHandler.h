#pragma once

#include "abstract/IEventHandler.h"
#include <functional>
#include <map>

class MockEventHandler : public IEventHandler {
public:
    MockEventHandler() = default;
    ~MockEventHandler() override = default;

    void runPlatform() override {}
    void setupQuartzEventTap() override {}
    
    void registerAppLaunch(std::function<void()> onLaunchCallback) override {
        appLaunchCallback = std::move(onLaunchCallback);
    }
    
    void registerAppTermination(std::function<void()> onTerminationCallback) override {
        appTerminationCallback = std::move(onTerminationCallback);
    }

    void focusLim() override {
        focusedWindow = "LIM";
    }

    void focusLive() override {
        focusedWindow = "Live";
    }

    void focusWindow(void* nativeWindowHandle) override {
        focusedWindow = "Custom";
        lastFocusedHandle = nativeWindowHandle;
    }

    void focusWindow(int windowID) override {
        focusedWindow = "Custom";
        lastFocusedID = windowID;
    }

    ERect getLiveBoundsRect() override {
        return liveBounds;
    }

    // Helper methods for testing
    void triggerAppLaunch() {
        if (appLaunchCallback) appLaunchCallback();
    }

    void triggerAppTermination() {
        if (appTerminationCallback) appTerminationCallback();
    }

    std::string getFocusedWindow() const {
        return focusedWindow;
    }

    void* getLastFocusedHandle() const {
        return lastFocusedHandle;
    }

    int getLastFocusedID() const {
        return lastFocusedID;
    }

    void setLiveBounds(const ERect& bounds) {
        liveBounds = bounds;
    }

private:
    std::function<void()> appLaunchCallback;
    std::function<void()> appTerminationCallback;
    std::string focusedWindow;
    void* lastFocusedHandle = nullptr;
    int lastFocusedID = -1;
    ERect liveBounds;
};
