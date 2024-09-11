#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "Types.h"

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

class WindowManager {
public:
    template<typename T>
    void registerWindowFactory(const std::string& windowName);

    void registerWindow(const std::string& windowName, std::function<void()> callback = nullptr);

    void* getWindowHandle(const std::string& windowName) const;

    void openWindow(const std::string& windowName);

    void closeWindow(const std::string& windowName);

    void toggleWindow(const std::string& windowName);

    // Check if a window is open
    bool isWindowOpen(const std::string& windowName) const;

private:
    // Factory function to create window instances based on window name
    std::unique_ptr<IWindow> createWindowInstance(const std::string& windowName);

    std::unordered_map<std::string, WindowData> windows_;  // Manage window instances and callbacks
    std::unordered_map<std::string, bool> windowStates_;
};

#endif
