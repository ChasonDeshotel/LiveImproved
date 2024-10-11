#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include "MacUtils.h"
#include "AXElement.h"
#include "AXFinder.h"

namespace AXInteraction {
    // Method to set text in the element
    void setText(AXUIElementRef element, const char* text) {
        CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
        AXUIElementSetAttributeValue(element, kAXValueAttribute, cfText);
        CFRelease(cfText);
    }

    // Method to focus the element
    void focusElement(AXUIElementRef element) {
        AXUIElementSetAttributeValue(element, kAXFocusedAttribute, kCFBooleanTrue);
        // TODO error checking
    }

    void closeSpecificWindow(AXElement element) {
        //AXUIElementRef element = static_cast<AXUIElementRef>(windowHandle);
        if (!element.isValid()) {
            return;
        }

        // Try to get the close button
        AXUIElementRef closeButton = nullptr;
        AXError error = AXUIElementCopyAttributeValue(element.getRef(), kAXCloseButtonAttribute, (CFTypeRef *)&closeButton);
        
        if (error != kAXErrorSuccess || !closeButton) {
            return;
        }

        // Press the close button
        error = AXUIElementPerformAction(closeButton, kAXPressAction);
        CFRelease(closeButton);

        if (error != kAXErrorSuccess) {
            return;
        }
    }

    void closeFocusedPluginWindow() {
        AXUIElementRef frontmostWindow = AXFinder::getFrontmostWindow();

        CFStringRef frontmostTitle = nullptr;
        AXError error = AXUIElementCopyAttributeValue(frontmostWindow, kAXTitleAttribute, (CFTypeRef *)&frontmostTitle);
        
        if (error != kAXErrorSuccess || !frontmostTitle) {
            logger->debug("Failed to get title of frontmost window.");
            return;
        }

        // Convert the frontmost window title to a std::string
        NSString *frontmostTitleNS = (__bridge NSString *)frontmostTitle;
        std::string frontmostTitleStr([frontmostTitleNS UTF8String]);

        // Get plugin windows from Live
        auto windows = AXFinder::getPluginWindowsFromLiveAX();
        if (windows.empty()) {
            logger->debug("No plugin windows found.");
            CFRelease(frontmostTitle);
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

        AXElement highestPlugin = windows[0];

        CGWindowID windowID;
        error = _AXUIElementGetWindow(highestPlugin.getRef(), &windowID);
        if (error == kAXErrorSuccess) {
            logger->debug("ID: " + std::to_string(static_cast<int>(windowID)));
    //        if (eventHandler_()->isWindowFocused(static_cast<int>(windowID))) {
    //            logger->debug("closing window");
                closeSpecificWindow(highestPlugin);
    //        }
        }
    }

}
