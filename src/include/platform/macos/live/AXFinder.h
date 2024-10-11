#pragma once

#import <ApplicationServices/ApplicationServices.h>

#include <vector>

#include "AXElement.h"

namespace AXFinder {
    AXUIElementRef appElement();

    AXUIElementRef getFrontmostWindow();

    CFArrayRef getAllWindows();

    AXUIElementRef findAXMain(AXUIElementRef parent);

    AXUIElementRef getTrackView();

    AXUIElementRef getTrackView();

    std::vector<AXUIElementRef> getTrackViewDevices();

    std::vector<AXUIElementRef> findElementsByType(AXUIElementRef parent, CFStringRef roleToFind, int level);

    AXUIElementRef findElementByAttribute(AXUIElementRef parent, CFStringRef valueToFind, CFStringRef searchAttribute, int level, int maxDepth = 5);

    AXUIElementRef findElementByIdentifier(AXUIElementRef parent, CFStringRef identifierToFind, int level);

    // Method to get the application's main window by its PID
    AXUIElementRef findApplicationWindow();

    AXUIElementRef getFocusedElement();

    std::vector<AXElement> getPluginWindowsFromLiveAX(int limit = -1);
    AXElement getFocusedPluginWindow();

    // Device On is the on/off switch
    std::vector<AXUIElementRef> getTrackViewDeviceCheckBoxes(AXUIElementRef deviceElement);

}
