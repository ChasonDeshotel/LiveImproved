#include "WindowManager.h"
#include "LogHandler.h"
#include "ApplicationManager.h"

#include "ContextMenu.h"
#include "SearchBox.h"

WindowManager::WindowManager() {}

// Factory function to create window instances dynamically based on the name
std::unique_ptr<IWindow> WindowManager::createWindowInstance(const std::string& windowName) {
    if (windowName == "ContextMenu") {
        return std::make_unique<ContextMenu>();
    } else if (windowName == "SearchBox") {
        return std::make_unique<SearchBox>();
    }
    return nullptr;
}

void WindowManager::registerWindow(const std::string& windowName, std::function<void()> callback) {
    if (windows_.find(windowName) == windows_.end()) {
        std::shared_ptr<IWindow> windowInstance = createWindowInstance(windowName);
        if (windowInstance != nullptr) {
            windows_[windowName] = {windowInstance, callback};
            windowStates_[windowName] = false;
        } else {
            throw std::runtime_error("Window class not found for: " + windowName);
        }
    }
}

void* WindowManager::getWindowHandle(const std::string& windowName) const {
    auto it = windows_.find(windowName);

    if (it != windows_.end()) {
        return it->second.window->getWindowHandle();
    } else {
        throw std::runtime_error("Window not found for: " + windowName);
    }
}

void WindowManager::openWindow(const std::string& windowName) {
    LogHandler::getInstance().debug("WM open called");

    auto it = windows_.find(windowName);

    // register the window if it doesn't yet exist
    if (it == windows_.end()) {
        LogHandler::getInstance().debug("window not registered, registering...");
        registerWindow(windowName, []() {});
        it = windows_.find(windowName); // Re-fetch the iterator
    }

    if (it->second.callback) {
        it->second.callback();
    }

    LogHandler::getInstance().debug("Current window state for " + windowName + ": " + std::to_string(windowStates_[windowName]));
    if (!windowStates_[windowName]) {
        // TODO: making some assumptions by setting this to true before
        // calling `open()`, but the context menu is blocking so...
        windowStates_[windowName] = true;
        it->second.window->open();
    }
}

void WindowManager::closeWindow(const std::string& windowName) {
    LogHandler::getInstance().debug("WM close called");

    auto it = windows_.find(windowName);
    if (it != windows_.end() && windowStates_[windowName]) {
        it->second.window->close();
    }
    windowStates_[windowName] = false;
    LogHandler::getInstance().debug("updated window state to close");
    LogHandler::getInstance().debug("close - Current window state for " + windowName + ": " + std::to_string(windowStates_[windowName]));
}

void WindowManager::toggleWindow(const std::string& windowName) {
    LogHandler::getInstance().debug("WM close called");

    auto it = windows_.find(windowName);

    // window isn't registered -- we'll create it (with open)
    if (it == windows_.end()) {
        LogHandler::getInstance().error("Window not found: " + windowName);
        openWindow(windowName);
        return;
    }

    bool isOpen = windowStates_[windowName];

    if (isOpen) {
        closeWindow(windowName);
        windowStates_[windowName] = !isOpen;
    } else {
        closeWindow(windowName);
        windowStates_[windowName] = !isOpen;
    }

    LogHandler::getInstance().debug("toggle - current window state for " + windowName + ": " + std::to_string(windowStates_[windowName]));
}

bool WindowManager::isWindowOpen(const std::string& windowName) const {
    auto it = windowStates_.find(windowName);
    return (it != windowStates_.end() && it->second);
}

// TODO
// closeAllWindows
// isWindowFocused
// focusWindow
// closeFocusedWindow
