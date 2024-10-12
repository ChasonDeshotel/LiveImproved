#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include <algorithm> // For std::reverse
#include <vector>

#include "LogGlobal.h"
#include "PID.h"
#include "MacUtils.h"

#include "AXAttribute.h"
#include "AXFinder.h"
#include "AXPrinter.h"
#include "AXWindow.h"

namespace AXFinder {
    AXUIElementRef appElement() {
        pid_t livePID = PID::getInstance().livePID();
        if (livePID == -1) {
            logger->error("Live is not running");
            return nullptr;
        }

        AXUIElementRef appElement = AXUIElementCreateApplication(livePID);
        if (appElement == nullptr) {
            std::cerr << "Failed to create application element for Live" << std::endl;
            return nullptr;
        }
        std::cout << "Successfully created application element" << std::endl;
        CFRetain(appElement);
        return appElement;
    }

    AXUIElementRef getFrontmostWindow() {
        AXUIElementRef frontmostWindow;

        AXError result = AXUIElementCopyAttributeValue(AXFinder::appElement(), kAXFocusedWindowAttribute, (CFTypeRef *)&frontmostWindow);
        
        if (result == kAXErrorSuccess && frontmostWindow) {
            logger->debug("Successfully obtained the frontmost window.");
        } else {
            logger->debug("Failed to get the frontmost window.");
        }

        return frontmostWindow;
    }

    CFArrayRef getAllWindows() {
        CFArrayRef windows;
        AXError result = AXUIElementCopyAttributeValue(AXFinder::appElement(), kAXWindowsAttribute, (CFTypeRef *)&windows);
        
        if (result == kAXErrorSuccess && windows) {
            CFIndex windowCount = CFArrayGetCount(windows);
            logger->debug("Number of windows: " + std::to_string(windowCount));
            
            for (CFIndex i = 0; i < windowCount; i++) {
                AXUIElementRef window = (AXUIElementRef)CFArrayGetValueAtIndex(windows, i);
                
                CFStringRef windowTitle;
                AXUIElementCopyAttributeValue(window, kAXTitleAttribute, (CFTypeRef *)&windowTitle);
                
                if (windowTitle) {
                    // Convert CFStringRef to std::string
                    NSString *nsString = (__bridge NSString *)windowTitle;
                    std::string title([nsString UTF8String]);
                    logger->debug("Window " + std::to_string(i) + ": " + title);
                    CFRelease(windowTitle);
                } else {
                    logger->debug("Window " + std::to_string(i) + ": (No Title)");
                }
            }
            return windows;
            CFRelease(windows);
        } else {
            logger->debug("Failed to get windows.");
        }
        return nullptr;
    }

    AXUIElementRef findAXMain(AXUIElementRef parent) {
        CFArrayRef children = nullptr;
        AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);

        if (error != kAXErrorSuccess || !children) {
            std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
            return nullptr;
        }

        // Ensure children is released at the end of the function
        std::unique_ptr<const __CFArray, decltype(&CFRelease)> childrenGuard(children, CFRelease);

        CFIndex count = CFArrayGetCount(children);
        for (CFIndex i = 0; i < count; ++i) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);

            // Check the AXMain attribute
            CFTypeRef attributeValue = nullptr;
            AXError attrError = AXUIElementCopyAttributeValue(child, kAXMainAttribute, &attributeValue);

            if (attrError == kAXErrorSuccess && attributeValue == kCFBooleanTrue) {
                CFRelease(attributeValue);
                std::cerr << "Match found: AXMain = true" << std::endl;
                CFRetain(child);
                return child;
            }

            if (attributeValue) {
                CFRelease(attributeValue);
            }

            // Recursively search in child elements
            AXUIElementRef found = findAXMain(child);
            if (found) {
                CFRetain(found);
                return found;
            }
        }

        return nullptr;
    }

    //  Identifier: TrackView.Device[0].TitleBar.DeviceOn
    //  Identifier: TrackView.Device[0].TitleBar.UnfoldDeviceParameters
    //  Identifier: TrackView.Device[0].TitleBar.PluginEdit
    //  Identifier: TrackView.Device[0].TitleBar.device_title
    //  Identifier: TrackView.Device[0].TitleBar.ConfigureMode

    // Found PluginEdit: TrackView.Device[2].TitleBar.PluginEdit
    // Attribute: AXIdentifier
    // Attribute: AXEnabled
    // Attribute: AXFrame
    // Attribute: AXDescription
    // Attribute: AXParent
    // Attribute: AXChildren
    // Attribute: AXFocused
    // Attribute: AXActivationPoint
    // Attribute: AXRole
    // Attribute: AXTopLevelUIElement
    // Attribute: AXHelp
    // Attribute: AXValue
    // Attribute: AXChildrenInNavigationOrder
    // Attribute: AXWindow
    // Attribute: AXRoleDescription
    // Attribute: AXTitle
    // Attribute: AXSubrole
    // Attribute: AXSize
    // Attribute: AXPosition
    AXUIElementRef getTrackView() {
        AXUIElementRef axMain = AXFinder::findAXMain(AXFinder::appElement());
        if (!AXAttribute::isValid(axMain)) {
            return nullptr;
        }

        CFStringRef valueToFind = CFSTR("TrackView");
        CFStringRef searchAttribute = kAXIdentifierAttribute;
        AXUIElementRef foundElement = AXFinder::findElementByAttribute(axMain, valueToFind, searchAttribute, 0, 1);
        CFRelease(axMain);
        CFRelease(searchAttribute);
        CFRelease(valueToFind);

        if (AXAttribute::isValid(foundElement)) {
            logger->info("found track view");
            CFRetain(foundElement);
            return foundElement;
        }
        return nullptr;
    }

    //  Identifier: TrackView.Device[0].TitleBar.DeviceOn
    //  Identifier: TrackView.Device[0].TitleBar.UnfoldDeviceParameters
    //  Identifier: TrackView.Device[0].TitleBar.PluginEdit
    //  Identifier: TrackView.Device[0].TitleBar.device_title
    //  Identifier: TrackView.Device[0].TitleBar.ConfigureMode

    // Found PluginEdit: TrackView.Device[2].TitleBar.PluginEdit
    // Attribute: AXIdentifier
    // Attribute: AXEnabled
    // Attribute: AXFrame
    // Attribute: AXDescription
    // Attribute: AXParent
    // Attribute: AXChildren
    // Attribute: AXFocused
    // Attribute: AXActivationPoint
    // Attribute: AXRole
    // Attribute: AXTopLevelUIElement
    // Attribute: AXHelp
    // Attribute: AXValue
    // Attribute: AXChildrenInNavigationOrder
    // Attribute: AXWindow
    // Attribute: AXRoleDescription
    // Attribute: AXTitle
    // Attribute: AXSubrole
    // Attribute: AXSize
    // Attribute: AXPosition

    std::vector<AXUIElementRef> getTrackViewDevices() {
        std::vector<AXUIElementRef> trackViewDevices;

        AXUIElementRef trackView = getTrackView();
        if (AXAttribute::isValid(trackView)) {

            CFArrayRef children = nullptr;
            AXError error = AXUIElementCopyAttributeValue(trackView, kAXChildrenAttribute, (CFTypeRef*)&children);
            CFRelease(trackView);

            if (error == kAXErrorSuccess && children) {
                CFIndex count = CFArrayGetCount(children);
                for (CFIndex i = 0; i < count; ++i) {
                    AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
                    CFRetain(child);
                    trackViewDevices.push_back(child);

                    CFStringRef identifier = nullptr;
                    if (AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
                        std::cout << " Identifier: ";
                        CFStringUtil::printCFString(identifier);
                        CFRelease(identifier);
                    } else {
                        std::cout << " Identifier: (none)" << std::endl;
                    }
                }
                CFRelease(children);
                return trackViewDevices;
            } else {
                std::cerr << "Failed to retrieve children or no children." << std::endl;
            }
        }

        return {};
    }

    std::vector<AXUIElementRef> findElementsByType(AXUIElementRef parent, CFStringRef roleToFind, int level) {
        std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

        std::vector<AXUIElementRef> matches;  // Vector to hold the matches

        if (level > 30) {
            std::cerr << "Max recursion depth reached at level " << level << std::endl;
            return matches;  // Return empty if depth is exceeded
        }

        if (!parent) {
            std::cerr << "Invalid parent element at level " << level << std::endl;
            return matches;
        }

        // Initialize children to nullptr
        CFArrayRef children = nullptr;

        // Retrieve the children of the parent element
        AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
        if (error != kAXErrorSuccess || !children) {
            std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
            return matches;  // Return empty if no children found
        }

        CFIndex count = CFArrayGetCount(children);
        std::cerr << "Number of children at level " << level << ": " << count << std::endl;

        for (CFIndex i = 0; i < count; i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
            if (!child) {
                std::cerr << "No valid child found at index " << i << std::endl;
                continue;  // Skip to next child if invalid
            }

            std::cerr << "Checking child at index " << i << " - Child element: " << child << std::endl;

            // Check the role (type) of the element
            CFTypeRef role = nullptr;
            AXError roleError = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, &role);
            if (roleError == kAXErrorSuccess && role) {
                CFStringRef roleStr = static_cast<CFStringRef>(role);
                if (CFStringCompare(roleStr, roleToFind, 0) == kCFCompareEqualTo) {
                    std::cerr << "Match found at index " << i << " - Level " << level << std::endl;
                    CFRetain(child);  // Retain the child before adding it to the vector
                    matches.push_back(child);  // Add the match to the vector
                    std::cerr << "Child retained and added to matches vector." << std::endl;
                }
                CFRelease(role);  // Always release the role attribute
            }

            // Recursively search in child elements and collect more matches
            std::vector<AXUIElementRef> childMatches = findElementsByType(child, roleToFind, level + 1);
            matches.insert(matches.end(), childMatches.begin(), childMatches.end());  // Append any matches found during recursion
        }

        // Release children array
        CFRelease(children);
        return matches;  // Return the collected matches
    }

    AXUIElementRef findElementByAttribute(AXUIElementRef parent, CFStringRef valueToFind, CFStringRef searchAttribute, int level, int maxDepth) {
        std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

        if (level > maxDepth) {
            std::cerr << "Max recursion depth reached at level " << level << std::endl;
            return nullptr;
        }

        if (!parent) {
            std::cerr << "Invalid parent element at level " << level << std::endl;
            return nullptr;
        }

        CFArrayRef children = nullptr;
        AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
        if (error != kAXErrorSuccess || !children) {
            std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
            return nullptr;
        }

        // Ensure children is released at the end of the function
        std::unique_ptr<const __CFArray, decltype(&CFRelease)> childrenGuard(children, CFRelease);

        CFIndex count = CFArrayGetCount(children);
        std::cerr << "Number of children at level " << level << ": " << count << std::endl;

        for (CFIndex i = 0; i < count; i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
            if (!AXAttribute::isValid(child)) {
                std::cerr << "No valid child found at index " << i << std::endl;
                continue;
            }

            // Get and print the title of each child
            CFStringRef title = nullptr;
            if (AXUIElementCopyAttributeValue(child, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess && title) {
                std::cout << std::string(level * 2, ' ') << "Title: ";
                CFStringUtil::printCFString(title);
                CFRelease(title);
            }

//            std::cerr << "Checking child at index " << i << " - Child element: " << child << std::endl;

            // Check the search attribute (e.g., AXIdentifier or AXRole)
            CFTypeRef attributeValue = nullptr;
            AXError attrError = AXUIElementCopyAttributeValue(child, searchAttribute, &attributeValue);
            if (attrError == kAXErrorSuccess && attributeValue) {
                CFStringRef attrStr = static_cast<CFStringRef>(attributeValue);
                if (CFStringCompare(attrStr, valueToFind, kCFCompareCaseInsensitive | kCFCompareNonliteral) == kCFCompareEqualTo) {
                    std::cerr << "Match found at index " << i << " - Level " << level << std::endl;
                    CFRelease(attributeValue);  // Release attribute value

                    // Return the child immediately
                    std::cerr << "Retaining and returning child at level " << level << std::endl;
                    CFRetain(child);
                    return child;
                }
                CFRelease(attributeValue);  // Release attribute value if not a match
            }

            // Recursively search in child elements
            std::cerr << "Recursing into child at index " << i << std::endl;
            AXUIElementRef found = findElementByAttribute(child, valueToFind, searchAttribute, level + 1);
            if (AXAttribute::isValid(found)) {
                std::cerr << "Found element during recursion at level " << level << std::endl;
                return found;
            } else {
                std::cerr << "No match found during recursion at level " << level << std::endl;
            }
        }

        std::cerr << "No match found at level " << level << std::endl;
        return nullptr;
    }

    AXUIElementRef findElementByIdentifier(AXUIElementRef parent, CFStringRef identifierToFind, int level) {
        std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

        if (level > 5) {
            std::cerr << "Max recursion depth reached at level " << level << std::endl;
            return nullptr;
        }

        if (!parent) {
            std::cerr << "Invalid parent element at level " << level << std::endl;
            return nullptr;
        }

        // Initialize children to nullptr
        CFArrayRef children = nullptr;

        // Retrieve the children of the parent element
        AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
        if (error != kAXErrorSuccess || !children) {
            std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
            return nullptr;  // If failed, return early to avoid accessing nullptr
        }

        CFIndex count = CFArrayGetCount(children);
        std::cerr << "Number of children at level " << level << ": " << count << std::endl;

        for (CFIndex i = 0; i < count; i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
            if (!child) {
                std::cerr << "No valid child found at index " << i << std::endl;
                continue;  // Skip to next child if invalid
            }

            std::cerr << "Checking child at index " << i << " - Child element: " << child << std::endl;

            // Check the identifier
            CFTypeRef identifier = nullptr;
            AXError idError = AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, &identifier);
            if (idError == kAXErrorSuccess && identifier) {
                CFStringRef identifierStr = static_cast<CFStringRef>(identifier);
                if (CFStringCompare(identifierStr, identifierToFind, 0) == kCFCompareEqualTo) {
                    std::cerr << "Match found at index " << i << " - Level " << level << std::endl;
                    CFRelease(identifier);  // Release identifier

                    // Return the child immediately
                    std::cerr << "Retaining and returning child at level " << level << std::endl;
                    CFRetain(child);
                    CFRelease(children);  // Release children array
                    return child;
                }
                CFRelease(identifier);  // Always release identifier if not a match
            }

            // Recursively search in child elements
            std::cerr << "Recursing into child at index " << i << std::endl;
            AXUIElementRef found = findElementByIdentifier(child, identifierToFind, level + 1);
            if (found) {
                std::cerr << "Found element during recursion at level " << level << std::endl;
                CFRelease(children);  // Release children array before returning
                return found;
            } else {
                std::cerr << "No match found during recursion at level " << level << std::endl;
            }
        }

        // Release children if nothing is found
        std::cerr << "Releasing children array at level " << level << std::endl;
        CFRelease(children);
        std::cerr << "No match found at level " << level << std::endl;
        return nullptr;
    }

    // Method to get the application's main window by its PID
    AXUIElementRef findApplicationWindow() {
        AXUIElementRef window = nullptr;
        AXUIElementRef appElement = AXFinder::appElement();
        if (AXAttribute::isValid(appElement)) {
            // Get the main window of the application
            AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute, (CFTypeRef*)&window);
            CFRelease(appElement);
            //std::cerr << "main window found - PID " + std::to_string(PID::getInstance().livePID()) << std::endl;
        } else {
            std::cerr << "no window found - PID " + std::to_string(PID::getInstance().livePID()) << std::endl;
        }
        return window;
    }

    AXUIElementRef getFocusedElement() {
        AXUIElementRef appElement = AXFinder::appElement();
        AXUIElementRef focusedElement = nullptr;

        AXError error = AXUIElementCopyAttributeValue(appElement, kAXFocusedUIElementAttribute, (CFTypeRef*)&focusedElement);
        if (error == kAXErrorSuccess && focusedElement != nullptr) {
            CFRetain(focusedElement);
            return(focusedElement);
        }

        return nullptr;
    }

    std::vector<AXUIElementRef> getPluginWindowsFromLiveAX(int limit) {
        logger->debug("getPluginWindowsFromLiveAX called");

        CFArrayRef appElementChildren = nullptr;  // Initialize to nullptr
        AXUIElementRef appElement = AXFinder::appElement();
        AXError error = AXUIElementCopyAttributeValues(appElement, kAXChildrenAttribute, 0, 100, &appElementChildren);

        if (error != kAXErrorSuccess || appElementChildren == nullptr) {
            logger->error("Failed to get appElementChildren of app. Error: " + std::to_string(error));
            if (appElement) CFRelease(appElement);  // Make sure to release appElement
            return {};
        }
        CFRelease(appElement);  // Safely release appElement after it's been used

        std::vector<AXUIElementRef> pluginWindowsFromLiveAX;
        CFIndex childCount = CFArrayGetCount(appElementChildren);
        logger->debug("child count: " + std::to_string(static_cast<int>(childCount)));

        for (CFIndex i = 0; i < childCount; i++) {
            if (limit != -1 && pluginWindowsFromLiveAX.size() >= static_cast<size_t>(limit)) {
                logger->debug("Limit reached, stopping collection of plugin windows.");
                break;
            }

            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(appElementChildren, i);
            if (AXWindow::isPluginWindow(child)) {
                logger->debug("Plugin Window: getCurrentPluginsWindow - Child " + std::to_string(static_cast<int>(i)));
                CFRetain(child);  // Retain the child before adding to the vector
                pluginWindowsFromLiveAX.push_back(child);
            } else {
                logger->debug("Not a plugin window:");
                // CFArrayGetValueAtIndex does not increment the retain count of the object it returns.
                // The returned AXUIElementRef is still owned by the CFArray. Thus, if you call CFRelease
                // on it without first calling CFRetain, you're essentially releasing an object that is 
                // still owned by the array, which can cause a crash or undefined behavior.
                // tl;dr: don't CFRelease(child) here
            }
        }

        if (appElementChildren) {
            CFRelease(appElementChildren);
        }
        
        return pluginWindowsFromLiveAX;
    }


    AXUIElementRef getFocusedPluginWindow() {
        std::vector<AXUIElementRef> pluginWindows = AXFinder::getPluginWindowsFromLiveAX();
        
        std::cout << "Number of plugin windows: " << pluginWindows.size() << std::endl;

        for (auto& window : pluginWindows) {
            bool isFocused = AXAttribute::isFocused(window);
            std::cout << "Window focus state: " << (isFocused ? "focused" : "not focused") << std::endl;

            if (isFocused) {
                return window;
            }
        }
        
        std::cout << "No focused window found" << std::endl;
        return nullptr;
    }

    // Device On is the on/off switch
    std::vector<AXUIElementRef> getTrackViewDeviceCheckBoxes(AXUIElementRef deviceElement) {
        if (!deviceElement) {
            std::cerr << "Error: deviceElement is null." << std::endl;
            return {};
        }

        std::vector<AXUIElementRef> foundCheckBoxes;

        AXPrinter::printAXTitle(deviceElement);

        CFArrayRef children = nullptr;
        AXError error = AXUIElementCopyAttributeValue(deviceElement, kAXChildrenAttribute, (CFTypeRef*)&children);

        if (error == kAXErrorSuccess && children) {
            CFIndex count = CFArrayGetCount(children);

            // Iterate over the children of TrackView.Device[i]
            for (CFIndex i = 0; i < count; i++) {
                AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);

                // Check if the child has the identifier TrackView.Device[i].TitleBar
                CFStringRef identifier = nullptr;
                if (AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
                    char identifierBuffer[256];
                    if (CFStringGetCString(identifier, identifierBuffer, sizeof(identifierBuffer), kCFStringEncodingUTF8)) {
                        std::string identifierStr(identifierBuffer);

                        if (identifierStr.find("PluginEdit") != std::string::npos) {
                            std::cout << "Found PluginEdit: " << identifierStr << std::endl;

                            CFRetain(child);
                            foundCheckBoxes.push_back(child);
                        }
                    }
                    CFRelease(identifier);
                }
            }

            CFRelease(children);

            if (!foundCheckBoxes.empty()) return foundCheckBoxes;
        }

        std::cerr << "Failed to find the DeviceOn element." << std::endl;
        return {};
    }
}
