#pragma once

#include <string>
#include <functional>
#include <memory>

class IWindow;

class IWindowManager {
public:
  virtual ~IWindowManager() = default;

  IWindowManager(const IWindowManager &) = default;
  IWindowManager(IWindowManager &&) = delete;
  IWindowManager &operator=(const IWindowManager &) = default;
  IWindowManager &operator=(IWindowManager &&) = delete;

  virtual void registerWindow(const std::string &windowName,
                              std::function<void()> callback) = 0;
  [[nodiscard]] virtual auto getWindowHandle(const std::string &windowName) const -> void * = 0;
  virtual void openWindow(const std::string &windowName) = 0;
  virtual void closeWindow(const std::string &windowName) = 0;
  virtual void toggleWindow(const std::string &windowName) = 0;
  [[nodiscard]] virtual auto isWindowOpen(const std::string &windowName) const -> bool = 0;

protected:
    virtual auto createWindowInstance(const std::string& windowName) -> std::unique_ptr<IWindow> = 0;
};
