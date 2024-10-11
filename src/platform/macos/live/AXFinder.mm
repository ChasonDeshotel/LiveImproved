#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include <algorithm> // For std::reverse
#include <vector>

#include "LogGlobal.h"
#include "PID.h"
#include "MacUtils.h"

#include "AXFinder.h"
#include "AXElement.h"

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
        return appElement;
    }

    AXUIElementRef getFrontmostWindow() {
        AXUIElementRef frontmostWindow;
        AXUIElementRef appElement = AXFinder::appElement();

        AXError result = AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute, (CFTypeRef *)&frontmostWindow);
        
        if (result == kAXErrorSuccess && frontmostWindow) {
            CFRelease(appElement);
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
        AXUIElementRef appElement = AXFinder::appElement();
        AXUIElementRef axMain = AXFinder::findAXMain(appElement);
        if (!axMain) {
            return nullptr;
        }
        //printAXTree(axMain, 0);
        //printAllAttributes(axMain);

        CFStringRef valueToFind = CFSTR("TrackView");
        CFStringRef searchAttribute = kAXIdentifierAttribute;
        AXUIElementRef foundElement = AXFinder::findElementByAttribute(axMain, valueToFind, searchAttribute, 0, 1);
        CFRelease(axMain);

        if (foundElement) {
            logger->info("found track view");
            //printAllAttributeValues(foundElement);
            CFRetain(foundElement);
            return foundElement;
        }
        return axMain;
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
    //    printAXTree(trackView, 0);

        if (trackView) {

            CFArrayRef children = nullptr;
            AXError error = AXUIElementCopyAttributeValue(trackView, kAXChildrenAttribute, (CFTypeRef*)&children);
            CFRelease(trackView);  // Release trackView after use

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
            if (!child) {
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

            std::cerr << "Checking child at index " << i << " - Child element: " << child << std::endl;

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
                    CFRetain(child);  // Retain to prevent accidental release
                    return child;
                }
                CFRelease(attributeValue);  // Release attribute value if not a match
            }

            // Recursively search in child elements
            std::cerr << "Recursing into child at index " << i << std::endl;
            AXUIElementRef found = findElementByAttribute(child, valueToFind, searchAttribute, level + 1);
            if (found) {
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
        AXUIElementRef appElement = AXFinder::appElement();
        
        AXUIElementRef window = nullptr;
        if (appElement) {
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
        AXElement appElement = AXElement(AXFinder::appElement());
        AXUIElementRef focusedElement = nullptr;

        AXError error = AXUIElementCopyAttributeValue(appElement.getRef(), kAXFocusedUIElementAttribute, (CFTypeRef*)&focusedElement);
        if (error == kAXErrorSuccess && focusedElement != nullptr) {
            AXElement(focusedElement).printValues();
            return(focusedElement);
        }

    //    if (error == kAXErrorSuccess && focusedElement != nullptr) {
    //        for (AXUIElementRef pluginWindow : pluginWindows) {
    //            if (CFEqual(focusedElement, pluginWindow)) {
    //                logger->info("One of the plugin windows is currently focused.");
    //                // You can take actions based on the focus state
    //            }
    //        }
    //        CFRelease(focusedElement);
    //    } else {
    //        logger->warn("No focused element found.");
    //    }

        return nullptr;
    }

    AXUIElementRef getFocusedPluginWindow() {
        std::vector<AXUIElementRef> pluginWindows = pluginWindows_;
        
        std::cout << "Number of plugin windows: " << pluginWindows.size() << std::endl;

        for (const auto& window : pluginWindows) {
            CFBooleanRef focusedValue;
            AXError error = AXUIElementCopyAttributeValue(window, kAXFocusedAttribute, (CFTypeRef*)&focusedValue);
            
            if (error == kAXErrorSuccess) {
                bool isFocused = CFBooleanGetValue(focusedValue);
                CFRelease(focusedValue);
                
                std::cout << "Window focus state: " << (isFocused ? "focused" : "not focused") << std::endl;

                if (isFocused) {
                    return window;
                }
            } else {
                std::cerr << "Failed to get focus state. Error: " << error << std::endl;
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
        
        std::vector<AXUIElementRef> checkBoxElements;

        AXElement(deviceElement).printTitle();

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

                        // Look for the TitleBar element
                        if (identifierStr.find("PluginEdit") != std::string::npos) {
                            std::cout << "Found PluginEdit: " << identifierStr << std::endl;

    //                        bool isChecked = isAXCheckBoxChecked(child);
    //                        logger->info("checkbox state: " + std::string(isChecked ? "true" : "false"));
                            CFRetain(child);
                            checkBoxElements.push_back(child);

                            CFRelease(identifier);
                        }
                    }
                }
            }
            CFRelease(children);
            if (checkBoxElements.size()) return checkBoxElements;
        }

        std::cerr << "Failed to find the DeviceOn element." << std::endl;
        return {};
    }

}
