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
    LiveInterface(
          std::function<std::shared_ptr<IEventHandler>()> eventHandler
    );

    ~LiveInterface() override;

    LiveInterface(const LiveInterface &) = default;
    LiveInterface(LiveInterface &&) = delete;
    LiveInterface &operator=(const LiveInterface &) = default;
    LiveInterface &operator=(LiveInterface &&) = delete;

    // TODO
    auto safelyFocusAfterDestroy(int windowIDToDestroy, int windowIDToFocus) -> void;

    auto setupPluginWindowChangeObserver(std::function<void()> callback) -> void override;
    auto removePluginWindowChangeObserver() -> void override;

    auto closeFocusedPlugin() -> void override;
    auto closeAllPlugins() -> void override;
    auto openAllPlugins() -> void override;
    auto tilePluginWindows() -> void override;

    auto isAnyTextFieldFocused() -> bool override;

private:
    auto isAnyTextFieldFocusedRecursive(AXUIElementRef parent, int level) -> bool;
    std::function<std::shared_ptr<IEventHandler>()> eventHandler_;
    auto setWindowBounds(AXUIElementRef window, int x, int y, int width, int height) -> void;
    std::map<AXUIElementRef, CGRect> cachedWindowBounds_;
    auto getWindowBounds(AXUIElementRef window) -> CGRect;

    bool windowCloseInProgress_ = false;
    AXObserverRef pluginWindowCreateObserver_;
    AXObserverRef pluginWindowDestroyObserver_;

    std::function<void()> createCallback_;
    static auto pluginWindowCreateCallback(AXObserverRef observer, AXUIElementRef element,
                                     CFStringRef notification, void* context) -> void;

    std::function<void()> destroyCallback_;
    static auto pluginWindowDestroyCallback(AXObserverRef observer, AXUIElementRef element,
                                     CFStringRef notification, void* context) -> void;

};
