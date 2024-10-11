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

    inline void closeFocusedPluginWindow() {
        AXUIElementRef frontmostWindow = getFrontmostWindow();

        CFStringRef frontmostTitle = nullptr;
        AXError error = AXUIElementCopyAttributeValue(frontmostWindow, kAXTitleAttribute, (CFTypeRef *)&frontmostTitle);
        
        if (error != kAXErrorSuccess || !frontmostTitle) {
            logger->debug("Failed to get title of frontmost window.");
            CFRelease(frontmostWindow);
            return;
        }

        // Convert the frontmost window title to a std::string
        NSString *frontmostTitleNS = (__bridge NSString *)frontmostTitle;
        std::string frontmostTitleStr([frontmostTitleNS UTF8String]);

        // Get plugin windows from Live
        auto windows = getPluginWindowsFromLiveAX();
        if (windows.empty()) {
            logger->debug("No plugin windows found.");
            CFRelease(frontmostTitle);
            CFRelease(frontmostWindow);
            return;
        }

        // if the frontmost window != a plugin window title, we bail
        bool found = false;
        for (const auto& pluginWindow : windows) {
            // Get the title of the plugin window
            CFStringRef pluginTitle = nullptr;
            AXError pluginError = AXUIElementCopyAttributeValue(pluginWindow, kAXTitleAttribute, (CFTypeRef *)&pluginTitle);

            if (pluginError == kAXErrorSuccess && pluginTitle) {
                // Convert the plugin window title to a std::string
                NSString *pluginTitleNS = (__bridge NSString *)pluginTitle;
                std::string pluginTitleStr([pluginTitleNS UTF8String]);

                // Compare titles
                if (frontmostTitleStr == pluginTitleStr) {
                    logger->debug("Frontmost window matches plugin window by title.");
                    found = true;
                    CFRelease(pluginTitle); // Release after use
                    break;
                }
                CFRelease(pluginTitle); // Release after use
            }
        }
        if (!found) {
            logger->debug("not found - returning");
            return;
        }

    //    printAXElementChildrenRecursively(getAppElement());

    //    //printAllAttributeValues(getAppElement());

        AXUIElementRef highestPlugin = windows[0];

        CGWindowID windowID;
        error = _AXUIElementGetWindow(highestPlugin, &windowID);
        if (error == kAXErrorSuccess) {
            logger->debug("ID: " + std::to_string(static_cast<int>(windowID)));
    //        if (eventHandler_()->isWindowFocused(static_cast<int>(windowID))) {
    //            logger->debug("closing window");
                closeSpecificWindow(highestPlugin);
    //        }
        }

        CFRelease(highestPlugin);
    }

    inline void tilePluginWindows() {
        AXUIElementRef trackView = getTrackView();
        std::vector<AXUIElementRef> trackViewDevices = getTrackViewDevices();

        for (const auto& device : trackViewDevices) {
    //        printAXIdentifier(device);
    //        printAXTree(device, 0);
            CFRetain(device);
            std::vector<AXUIElementRef> checkboxes = getTrackViewDeviceCheckBoxes(device);

            for (const auto& checkbox : checkboxes) {
                toggleOffOnAXCheckBox(checkbox);
                if (checkbox) CFRelease(checkbox);
                usleep(20000);
            }

            CFRelease(device);
        }

        return;

    //    if (trackView) {
    //        printAXTree(trackView, 0);
    //        CFRelease(trackView);
    //    }
    //
    //    return;


    //    printAXTree(getAppElement(), 0);
    //    return;
    //    CFStringRef valueToFind = CFSTR("Trackview");
    //    AXUIElementRef deviceDetailGroup = findElementByIdentifier(getAppElement(), valueToFind, 0);
    //    if (deviceDetailGroup) {
    //        logger->info("found device detail group");
    //        printAXTree(deviceDetailGroup, 0);
    //    }
    //    return;

        std::vector<AXUIElementRef> pluginWindows = getPluginWindowsFromLiveAX();
        if (pluginWindows.empty()) return;

        std::reverse(pluginWindows.begin(), pluginWindows.end());

        CGRect screenBounds = [[NSScreen mainScreen] frame];
        int screenWidth = screenBounds.size.width;
        int screenHeight = screenBounds.size.height;

        // Calculate the total area of all plugin windows
        double totalArea = 0;
        for (const auto& window : pluginWindows) {
            CGRect bounds = getWindowBounds(window);
            totalArea += bounds.size.width * bounds.size.height;
        }

        // Calculate the scaling factor to fit all windows on the screen
        //double scaleFactor = std::sqrt((screenWidth * screenHeight) / totalArea);
        double scaleFactor = 1;

        // Calculate the number of rows and columns
        int numWindows = static_cast<int>(pluginWindows.size());
        int numCols = std::ceil(std::sqrt(numWindows));
        int numRows = std::ceil(static_cast<double>(numWindows) / numCols);

        // Arrange windows
        int currentX = 0;
        int currentY = 0;
        int maxRowHeight = 0;

        for (int i = 0; i < numWindows; ++i) {
            CGRect originalBounds = getWindowBounds(pluginWindows[i]);
            int scaledWidth = std::round(originalBounds.size.width * scaleFactor);
            int scaledHeight = std::round(originalBounds.size.height * scaleFactor);

            // If adding this window would go past the right edge of the screen, move to the next row
            if (currentX + scaledWidth > screenWidth) {
                currentX = 0;  // Reset X to the left
                currentY += maxRowHeight;  // Move down to the next row
                maxRowHeight = 0;  // Reset row height for the new row
            }

            // If the next row would go off the screen, break the loop (optional if you want to stop tiling)
            if (currentY + scaledHeight > screenHeight) {
                break;
            }

            // Set the window bounds with the current calculated position
            setWindowBounds(pluginWindows[i], currentX, currentY, scaledWidth, scaledHeight);

            // Move the X position for the next window
            currentX += scaledWidth;

            // Keep track of the tallest window in the row
            maxRowHeight = std::max(maxRowHeight, scaledHeight);
        }
    }
}
