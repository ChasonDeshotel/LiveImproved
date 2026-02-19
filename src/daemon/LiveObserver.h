#pragma once
#include <functional>

#include <ApplicationServices/ApplicationServices.h>
#ifdef __OBJC__
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef __OBJC__
@class NSView;
@class NSWindow;
@class NSDictionary;
@class NSArray;
#else
// Forward declare as empty structs for pure C++ compatibility
struct NSView;
struct NSWindow;
struct NSDictionary;
struct NSArray;
#endif

class LiveObserver {
public:
    LiveObserver() = default;

    ~LiveObserver();

    LiveObserver(const LiveObserver &) = default;
    LiveObserver(LiveObserver &&) = delete;
    LiveObserver &operator=(const LiveObserver &) = default;
    LiveObserver &operator=(LiveObserver &&) = delete;
    static void registerAppLaunch(std::function<void()> onLaunchCallback);
    static void registerAppTermination(std::function<void()> onTerminationCallback);
};
