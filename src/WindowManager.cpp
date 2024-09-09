#include "WindowManager.h"
#include "ContextMenu.h"

// Factory function to create window instances dynamically based on the name
std::shared_ptr<IWindow> WindowManager::createWindowInstance(const std::string& windowName) {
    if (windowName == "ContextMenu") {
        return std::make_shared<ContextMenu>();  // Create and return the correct window class
    }
    // Add more mappings for other window names as needed
    return nullptr;  // Return null if the window name doesn't match any class
}

void WindowManager::registerWindow(const std::string& windowName, std::function<void()> callback) {
    if (windows_.find(windowName) == windows_.end()) {
        // Create the window instance if it doesn't exist
        std::shared_ptr<IWindow> windowInstance = createWindowInstance(windowName);
        if (windowInstance != nullptr) {
            windows_[windowName] = {windowInstance, callback};
            windowStates_[windowName] = false;  // Initially set to closed
        } else {
            // Log error or handle the case where the windowName doesn't match any class
            throw std::runtime_error("Window class not found for: " + windowName);
        }
    }
}

// Open a window, registering it first if it doesn't exist
void WindowManager::openWindow(const std::string& windowName) {
    auto it = windows_.find(windowName);

    // If the window isn't registered, register it with a default callback
    if (it == windows_.end()) {
        registerWindow(windowName, []() {
            // Default callback if none is provided
        });
        it = windows_.find(windowName);  // Re-fetch the iterator
    }

    // Call the stored callback if one was provided
    if (it->second.callback) {
        it->second.callback();
    }

    // Open the window if it's not already open
    if (!windowStates_[windowName]) {
        it->second.window->open();
        windowStates_[windowName] = true;
    }
}

// Close a window if it exists and is open
void WindowManager::closeWindow(const std::string& windowName) {
    auto it = windows_.find(windowName);
    if (it != windows_.end() && windowStates_[windowName]) {
        it->second.window->close();
        windowStates_[windowName] = false;
    }
}

// Check if a window is open
bool WindowManager::isWindowOpen(const std::string& windowName) const {
    auto it = windowStates_.find(windowName);
    return (it != windowStates_.end() && it->second);
}

