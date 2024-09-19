#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <stdexcept>

class ApplicationManager;
class IWindow;
class ILogHandler;
class WindowData;

class WindowManager {
public:
    WindowManager();

    // TODO remove unused "override callback" param
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

    std::unordered_map<std::string, WindowData> windows_;
    std::unordered_map<std::string, bool> windowStates_;
    std::shared_ptr<ILogHandler> logHandler_;
 //   std::unordered_map<std::string, WindowFactory> windowFactories_;
//    std::unordered_map<std::string, struct { std::unique_ptr<IWindow> window; std::function<void()> callback; }> windows_;
};
