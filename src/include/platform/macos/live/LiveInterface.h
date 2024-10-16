#pragma once

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>  // Cocoa for Objective-C code (NSView, NSWindow)
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>
#endif

#include <functional>
#include <map>
#include <set>
#include <vector>

#include "ILiveInterface.h"

class IEventHandler;

class LiveInterface : public ILiveInterface {
public:
  LiveInterface(std::function<std::shared_ptr<IEventHandler>()> eventHandler);

  ~LiveInterface() override;

  LiveInterface(const LiveInterface &) = default;
  LiveInterface(LiveInterface &&) = delete;
  LiveInterface &operator=(const LiveInterface &) = default;
  LiveInterface &operator=(LiveInterface &&) = delete;

  // TODO
  void safelyFocusAfterDestroy(int windowIDToDestroy, int windowIDToFocus);

  void setupPluginWindowChangeObserver(std::function<void()> callback) override;
  void removePluginWindowChangeObserver() override;

  void closeFocusedPlugin() override;
  void closeAllPlugins() override;
  void openAllPlugins() override;
  void tilePluginWindows() override;

private:
    std::function<std::shared_ptr<IEventHandler>()> eventHandler_;
    void setWindowBounds(AXUIElementRef window, int x, int y, int width, int height);
    std::map<AXUIElementRef, CGRect> cachedWindowBounds_;
    CGRect getWindowBounds(AXUIElementRef window);

    bool windowCloseInProgress_ = false;
    AXObserverRef pluginWindowCreateObserver_;
    AXObserverRef pluginWindowDestroyObserver_;

    std::function<void()> createCallback_;
    static void pluginWindowCreateCallback(AXObserverRef observer, AXUIElementRef element,
                                     CFStringRef notification, void* context);

    std::function<void()> destroyCallback_;
    static void pluginWindowDestroyCallback(AXObserverRef observer, AXUIElementRef element,
                                     CFStringRef notification, void* context);

};
