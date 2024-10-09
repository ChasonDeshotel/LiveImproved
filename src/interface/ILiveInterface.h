#pragma once

#include <functional>
class ILogHandler;

using WindowHandle = const void*;

class ILiveInterface {
public:
  virtual ~ILiveInterface() = default;

  ILiveInterface(const ILiveInterface &) = default;
  ILiveInterface(ILiveInterface &&) = delete;
  ILiveInterface &operator=(const ILiveInterface &) = default;
  ILiveInterface &operator=(ILiveInterface &&) = delete;

  virtual void closeFocusedPluginWindow() = 0;
  virtual void closeSpecificWindow(WindowHandle element) = 0;

  virtual void setupPluginWindowChangeObserver(std::function<void()> callback) = 0;
  virtual void removePluginWindowChangeObserver() = 0;
};
