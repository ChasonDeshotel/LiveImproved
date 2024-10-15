#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include "MacUtils.h"
#include "AXAttribute.h"
#include "AXComponents.h"
#include "AXFinder.h"
#include "AXPrinter.h"

namespace AXInteraction {
    void setText(AXUIElementRef element, const char* text) {
        CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
        AXUIElementSetAttributeValue(element, kAXValueAttribute, cfText);
        CFRelease(cfText);
    }

    void focusElement(AXUIElementRef element) {
        AXUIElementSetAttributeValue(element, kAXFocusedAttribute, kCFBooleanTrue);
        // TODO error checking
    }

    void closeSpecificWindow(AXUIElementRef element) {
        //AXUIElementRef element = static_cast<AXUIElementRef>(windowHandle);
        if (!AXAttribute::isValid(element)) {
            return;
        }

        // Try to get the close button
        AXUIElementRef closeButton = nullptr;
        AXError error = AXUIElementCopyAttributeValue(element, kAXCloseButtonAttribute, castutil::toCFTypeRef(&closeButton));
        
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

    std::vector<AXUIElementRef> pluginWindowsOrNullIfLiveIsFrontmost() {
        // this is the check to make sure a plugin window is focused
        // we do this by 
        AXUIElementRef frontmostWindow = AXFinder::getFrontmostWindow();

        CFStringRef frontmostTitle = nullptr;
        AXError error = AXUIElementCopyAttributeValue(frontmostWindow, kAXTitleAttribute, castutil::toCFTypeRef(&frontmostTitle));

        if (error != kAXErrorSuccess || !frontmostTitle) {
            logger->debug("Failed to get title of frontmost window.");
            return {};
        }

        // Convert the frontmost window title to a std::string
        auto *frontmostTitleNS = (__bridge NSString *)frontmostTitle;
        std::string frontmostTitleStr([frontmostTitleNS UTF8String]);

        // Get plugin windows from Live
        auto pluginWindows = AXFinder::getPluginWindowsFromLiveAX();
        if (pluginWindows.empty()) {
            logger->debug("No plugin windows found.");
            CFRelease(frontmostTitle);
            return {};
        }

        // compare the title of the frontmost window to all of the plugins
        // if the frontmost title doesn't match any of the plugins, we
        // assume Live has focus. Yes, it's janky. I think it's the best
        // we have though
        bool found = false;
        for (const auto& pluginWindow : pluginWindows) {
            CFStringRef pluginTitle = nullptr;
            AXError pluginError = AXUIElementCopyAttributeValue(pluginWindow, kAXTitleAttribute, castutil::toCFTypeRef(&pluginTitle));

            if (pluginError == kAXErrorSuccess && pluginTitle) {
                // Convert the plugin window title to a std::string
                auto *pluginTitleNS = (__bridge NSString *)pluginTitle;
                std::string pluginTitleStr([pluginTitleNS UTF8String]);

                // plugin is top and focused
                if (frontmostTitleStr == pluginTitleStr) {
                    logger->debug("Frontmost window matches plugin window by title.");
                    found = true;
                    CFRelease(pluginTitle);
                    return pluginWindows;
                } else {
                    CFRelease(pluginTitle);
                }
            }
        }

        return {};
    }

    void closeFocusedPlugin() {
        std::vector<AXUIElementRef> windows = pluginWindowsOrNullIfLiveIsFrontmost();
        if (windows.empty()) {
            logger->warn("no windows to close");
            return;
        }

        AXUIElementRef highestPlugin = windows[0];

        CGWindowID windowID = -1;
        AXError error = _AXUIElementGetWindow(highestPlugin, &windowID);
        if (error == kAXErrorSuccess) {
            logger->debug("ID: " + std::to_string(static_cast<int>(windowID)));
            closeSpecificWindow(highestPlugin);
        }
    }

    // simulates press on the close button
    void closeAllPlugins() {
        auto pluginWindows = AXFinder::getPluginWindowsFromLiveAX();
        if (pluginWindows.empty()) {
            logger->debug("No plugin windows found.");
            return;
        }
        // Get plugin windows from Live

        // now we get to close all of the windows
        for (const auto& plugin : pluginWindows) {
            CGWindowID windowID = -1;
            AXError error = _AXUIElementGetWindow(plugin, &windowID);
            if (error == kAXErrorSuccess) {
                logger->debug("ID: " + std::to_string(static_cast<int>(windowID)));
                closeSpecificWindow(plugin);
                CFRelease(plugin);
            }
        }
    }

    // iterate through device checkboxes
    void openAllPlugins() {
        AXUIElementRef trackView = AXFinder::getTrackView();
        logger->error("\n\n\n\n\n\n");
        if (!AXAttribute::isValid(trackView)) {
            logger->warn("unable to find valid TrackView");
        }

        // find the devices in TrackView and toggle the ones that are enabled
        // to correctly order the plugins reported by Live AX
        std::vector<AXUIElementRef> trackViewDevices = AXFinder::getTrackViewDevices();
        for (const auto& device : trackViewDevices) {
            CFRetain(device);
            std::vector<AXUIElementRef> checkboxes = AXFinder::getTrackViewDeviceCheckBoxes(device);
            if (checkboxes.empty()) {
                logger->warn("couldn't find device on/off checkboxes");
                return;
            }
            CFRelease(device);

            for (const auto& checkbox : checkboxes) {
                AXPrinter::printAXIdentifier(checkbox);
                AXCheckBox::toggleOn(checkbox);
                usleep(10000);
                CFRelease(checkbox);
            }
        }
    }
}
