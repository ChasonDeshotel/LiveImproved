#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

namespace AXPluginWindows {
    inline std::vector<AXUIElementRef> getPluginWindowsFromLiveAX(int limit = -1) {
        logger->debug("getPluginWindowsFromLiveAX called");

        CFArrayRef children;
        AXUIElementRef appElement = getAppElement();
        AXError error = AXUIElementCopyAttributeValues(appElement, kAXChildrenAttribute, 0, 100, &children);
        if (error != kAXErrorSuccess) {
            logger->error(&"Failed to get children of app. Error: " [error]);
            CFRelease(appElement);
            return {};
        }

        std::vector<AXUIElementRef> pluginWindowsFromLiveAX;
        CFIndex childCount = CFArrayGetCount(children);
        logger->debug("child count: " + std::to_string(static_cast<int>(childCount)));

        for (CFIndex i = 0; i < childCount; i++) {
            if (limit != -1 && pluginWindowsFromLiveAX.size() >= static_cast<size_t>(limit)) {
                logger->debug("Limit reached, stopping collection of plugin windows.");
                break;
            }
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
            if (isPluginWindow(child)) {
                logger->debug("getCurrentPluginsWindow - Child " + std::to_string(static_cast<int>(i)));
                printElementInfo(child, "  plugin window: ");
                CFRetain(child);
                pluginWindowsFromLiveAX.push_back(child);
            } else {
                printElementInfo(child, "  other child: ");
            }
        }

        if (appElement) CFRelease (appElement);
        if (children) CFRelease (children);
        
        return pluginWindowsFromLiveAX;
    }

    inline bool isAnyPluginWindowFocused() {
        AXUIElementRef focusedPlugin = getFocusedPluginWindow();
        return focusedPlugin != nullptr;
    }


}
