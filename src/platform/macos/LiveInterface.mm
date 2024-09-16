#include "LiveInterface.h"
#include "PID.h"
#include <iostream>

// Constructor
LiveInterface::LiveInterface() {}

// Destructor
LiveInterface::~LiveInterface() {}

//AXUIElementRef LiveInterface::getMainWindow() {
//    return mainWindow_;
//}

// Method to get the application's main window by its PID
AXUIElementRef LiveInterface::findApplicationWindow() {
    AXUIElementRef appElement = AXUIElementCreateApplication(PID::getInstance().livePID());
    
    AXUIElementRef window = nullptr;
    if (appElement) {
        // Get the main window of the application
        AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute, (CFTypeRef*)&window);
        CFRelease(appElement);
        std::cerr << "\n\nmain window found - PID " + std::to_string(PID::getInstance().livePID()) << std::endl;
    } else {
        std::cerr << "\n\nno window found - PID " + std::to_string(PID::getInstance().livePID()) << std::endl;
    }
    mainWindow_ = window;
    return window;
}

//AXUIElementRef LiveInterface::findSearchBox() {
//    mainWindow_
//}
//
//AXUIElementRef LiveInterface::getFocusedElement() {
//    mainWindow_.kAXFocusedUIElementAttribute
//}
void LiveInterface::printFocusedElementInChildren(AXUIElementRef parent) {
    if (!parent) {
        std::cerr << "No parent element provided." << std::endl;
        return;
    }

    // Retrieve the children of the parent element
    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);

    if (error != kAXErrorSuccess || !children) {
        std::cerr << "Unable to retrieve children for the element or no children." << std::endl;
        return;
    }

    CFIndex count = CFArrayGetCount(children);
    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        
        // Check if this child is focused
        AXUIElementRef focusedElement = nullptr;
        AXError focusedError = AXUIElementCopyAttributeValue(child, kAXFocusedUIElementAttribute, (CFTypeRef*)&focusedElement);

        if (focusedError == kAXErrorSuccess && focusedElement) {
            std::cout << "Focused element found!" << std::endl;
            printElementInfo(focusedElement);  // Print the info for the focused element
            CFRelease(focusedElement);
            CFRelease(children);  // Release children array when we find the focused element
            return;
        } else {
            // Recursively search in child elements
            printFocusedElementInChildren(child);
        }
    }

    CFRelease(children);  // Release children array if no focused element is found
}

void printChildren(AXUIElementRef element, int level = 0) {
    if (!element) {
        std::cerr << "Invalid AXUIElementRef." << std::endl;
        return;
    }

    // Indentation for the hierarchy level
    std::string indent(level * 2, ' ');

    // Get the role of the element
    CFTypeRef role = nullptr;
    AXError roleError = AXUIElementCopyAttributeValue(element, kAXRoleAttribute, &role);
    if (roleError == kAXErrorSuccess && role) {
        CFStringRef roleStr = static_cast<CFStringRef>(role);
        char buffer[256];
        if (CFStringGetCString(roleStr, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
            std::cout << indent << "Role: " << buffer << std::endl;
        }
        CFRelease(role);
    } else {
        std::cerr << indent << "Failed to get role for element. Error: " << roleError << std::endl;
        return;  // If we can't get the role, there's no point continuing
    }

    // Optionally print other attributes (like title)
    CFTypeRef title = nullptr;
    AXError titleError = AXUIElementCopyAttributeValue(element, kAXTitleAttribute, &title);
    if (titleError == kAXErrorSuccess && title) {
        CFStringRef titleStr = static_cast<CFStringRef>(title);
        char titleBuffer[256];
        if (CFStringGetCString(titleStr, titleBuffer, sizeof(titleBuffer), kCFStringEncodingUTF8)) {
            std::cout << indent << "Title: " << titleBuffer << std::endl;
        }
        CFRelease(title);
    } else if (titleError != kAXErrorNoValue) {
        std::cerr << indent << "Failed to get title for element. Error: " << titleError << std::endl;
    }

    // Print the identifier if available
    CFTypeRef identifier = nullptr;
    AXError idError = AXUIElementCopyAttributeValue(element, kAXIdentifierAttribute, &identifier);
    if (idError == kAXErrorSuccess && identifier) {
        CFStringRef identifierStr = static_cast<CFStringRef>(identifier);
        char identifierBuffer[256];
        if (CFStringGetCString(identifierStr, identifierBuffer, sizeof(identifierBuffer), kCFStringEncodingUTF8)) {
            std::cout << indent << "Identifier: " << identifierBuffer << std::endl;
        }
        CFRelease(identifier);
    } else if (idError != kAXErrorNoValue) {
        std::cerr << indent << "Failed to get identifier for element. Error: " << idError << std::endl;
    }

    // Get the children of the element
    CFArrayRef children = nullptr;
    AXError childrenError = AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, (CFTypeRef*)&children);
    if (childrenError == kAXErrorSuccess && children) {
        CFIndex count = CFArrayGetCount(children);
        std::cout << indent << "Number of children: " << count << std::endl;

        // Recursively print each child element
        for (CFIndex i = 0; i < count; i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
            CFRetain(child);  // Retain the child to ensure it stays valid
            printChildren(child, level + 1);  // Increase the level for indentation
            CFRelease(child);  // Release the child after use
        }
        CFRelease(children);  // Release the array after use
    } else if (childrenError == kAXErrorInvalidUIElement) {
        std::cerr << indent << "No children or unable to retrieve children for this element. Error: " << childrenError << std::endl;
    } else {
        std::cerr << indent << "Failed to retrieve children. Error: " << childrenError << std::endl;
    }
}

bool LiveInterface::isAnyTextFieldFocusedRecursive(AXUIElementRef parent, int level) {
//    std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

    if (level > 10) {
//        std::cerr << "Max recursion depth reached at level " << level << std::endl;
        return false;
    }

    if (!parent) {
        std::cerr << "Invalid parent element at level " << level << std::endl;
        return false;
    }

    // Initialize children to nullptr
    CFArrayRef children = nullptr;

    // Retrieve the children of the parent element
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
    if (error != kAXErrorSuccess || !children) {
//        std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
        return false;  // Return false if no children found
    }

    CFIndex count = CFArrayGetCount(children);
//    std::cerr << "Number of children at level " << level << ": " << count << std::endl;

    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        if (!child) {
//            std::cerr << "No valid child found at index " << i << std::endl;
            continue;  // Skip to next child if invalid
        }

        // Check if the element is a text field
        CFTypeRef role = nullptr;
        AXError roleError = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, &role);
        if (roleError == kAXErrorSuccess && role) {
            CFStringRef roleStr = static_cast<CFStringRef>(role);
            if (CFStringCompare(roleStr, CFSTR("AXTextField"), 0) == kCFCompareEqualTo) {
                CFRelease(role);

                // Check if the text field is focused
                CFTypeRef focusedValue;
                AXError focusedError = AXUIElementCopyAttributeValue(child, kAXFocusedAttribute, &focusedValue);
                if (focusedError == kAXErrorSuccess && focusedValue == kCFBooleanTrue) {
                    std::cerr << "Focused text field found at level " << level << std::endl;
                    CFRelease(children);
                    return true;  // Exit early if a focused text field is found
                }
                CFRelease(focusedValue);
            } else {
                CFRelease(role);
            }
        }

        // Recursively search in child elements
        if (isAnyTextFieldFocusedRecursive(child, level + 1)) {
            CFRelease(children);  // Release children array before returning
            return true;  // Exit early if a focused text field is found during recursion
        }
    }

    // Release children if nothing is found
    CFRelease(children);
    return false;  // No focused text field found
}

bool LiveInterface::isAnyTextFieldFocused() {
    AXUIElementRef window = findApplicationWindow();
    if (!window) {
        std::cerr << "Window is null or invalid." << std::endl;
        return false;
    }

    return isAnyTextFieldFocusedRecursive(window, 0);
}

// search box
//    if (window) {
//        AXUIElementRef liveSearchBox = findElementByIdentifier(window, cfIdentifier, 0);
//
//        CFRelease(window);
//        CFRelease(cfIdentifier);
//
//        if (liveSearchBox) {
//            std::cerr << "Search box found." << std::endl;
//            printAllAttributes(liveSearchBox);
//            CFTypeRef focusedValue;
//            AXUIElementCopyAttributeValue(liveSearchBox, kAXFocusedAttribute, &focusedValue);
//            if (focusedValue == kCFBooleanTrue) {
//                std::cout << "Element is focused." << std::endl;
//            } else {
//                std::cout << "Element is not focused." << std::endl;
//            }
//            CFRelease(liveSearchBox);
//        }
//    } else {
//        std::cerr << "Search box not found." << std::endl;
//    }

void LiveInterface::printAllAttributes(const AXUIElementRef element) {
    CFArrayRef attributeNames = nullptr;
    AXError error = AXUIElementCopyAttributeNames(element, &attributeNames);
    if (error == kAXErrorSuccess && attributeNames) {
        CFIndex count = CFArrayGetCount(attributeNames);
        for (CFIndex i = 0; i < count; i++) {
            CFStringRef attributeName = static_cast<CFStringRef>(CFArrayGetValueAtIndex(attributeNames, i));
            char buffer[256];
            if (CFStringGetCString(attributeName, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
                std::cout << "Attribute: " << buffer << std::endl;
            }
        }
        CFRelease(attributeNames);
    } else {
        std::cerr << "Unable to retrieve attributes for this element." << std::endl;
    }
}

std::vector<AXUIElementRef> LiveInterface::findElementsByType(AXUIElementRef parent, CFStringRef roleToFind, int level) {
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


AXUIElementRef LiveInterface::findElementByAttribute(AXUIElementRef parent, CFStringRef valueToFind, CFStringRef searchAttribute, int level) {
    std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

    if (level > 3) {
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

        // Check the search attribute (e.g., AXIdentifier or AXRole)
        CFTypeRef attributeValue = nullptr;
        AXError attrError = AXUIElementCopyAttributeValue(child, searchAttribute, &attributeValue);
        if (attrError == kAXErrorSuccess && attributeValue) {
            CFStringRef attrStr = static_cast<CFStringRef>(attributeValue);
            if (CFStringCompare(attrStr, valueToFind, 0) == kCFCompareEqualTo) {
                std::cerr << "Match found at index " << i << " - Level " << level << std::endl;
                CFRelease(attributeValue);  // Release attribute value

                // Return the child immediately
                std::cerr << "Retaining and returning child at level " << level << std::endl;
                CFRetain(child);
                CFRelease(children);  // Release children array
                return child;
            }
            CFRelease(attributeValue);  // Always release attribute value if not a match
        }

        // Recursively search in child elements
        std::cerr << "Recursing into child at index " << i << std::endl;
        AXUIElementRef found = findElementByAttribute(child, valueToFind, searchAttribute, level + 1);
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


AXUIElementRef LiveInterface::findElementByIdentifier(AXUIElementRef parent, CFStringRef identifierToFind, int level) {
    std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

    if (level > 3) {
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

void LiveInterface::searchFocusedTextField(AXUIElementRef parent) {
    if (!parent) {
        std::cerr << "No parent element provided." << std::endl;
        return;
    }

    // Retrieve the children of the parent element
    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);

    if (error != kAXErrorSuccess || !children) {
        std::cerr << "Unable to retrieve children for the element or no children found." << std::endl;
        return;
    }

    CFIndex count = CFArrayGetCount(children);
    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        
        // Check if the child has the AXRoleAttribute
        CFTypeRef role;
        AXError roleError = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, &role);
        if (roleError == kAXErrorSuccess && role) {
            CFStringRef roleStr = static_cast<CFStringRef>(role);

            // Check if the child is a text field (AXTextFieldRole or TAxEditFieldElement)
            if (CFStringCompare(roleStr, kAXTextFieldRole, 0) == kCFCompareEqualTo) {
                // Check if this text field is focused
                CFTypeRef isFocused = nullptr;
                AXError focusedError = AXUIElementCopyAttributeValue(child, kAXFocusedAttribute, &isFocused);
                
                if (focusedError == kAXErrorSuccess && isFocused == kCFBooleanTrue) {
                    std::cout << "Focused text field found!" << std::endl;
                    printElementInfo(child);  // Print info about the focused text field
                    CFRelease(role);
                    CFRelease(children);
                    return;
                }
                CFRelease(isFocused);
            }
            CFRelease(role);
        }

        // Recursively search in child elements
        searchFocusedTextField(child);
    }

    CFRelease(children);
}

void LiveInterface::printFocusedChildElementInfo(const AXUIElementRef mainWindow) {
    if (!mainWindow) {
        std::cerr << "No main window element provided." << std::endl;
        return;
    }

    // Get the focused element in the application (recursively check children)
    AXUIElementRef focusedElement = nullptr;
    AXError error = AXUIElementCopyAttributeValue(mainWindow, kAXFocusedUIElementAttribute, (CFTypeRef*)&focusedElement);

    if (error != kAXErrorSuccess || !focusedElement) {
        std::cerr << "No focused element in the main window." << std::endl;
        return;
    }

    // Print info about the focused element
    printElementInfo(focusedElement);

    // Release the focused element
    CFRelease(focusedElement);
}

void LiveInterface::printElementInfo(const AXUIElementRef element) {
    if (!element) {
        std::cerr << "No element to print info." << std::endl;
        return;
    }

    // Retrieve and print the role of the element
    CFTypeRef role;
    AXError error = AXUIElementCopyAttributeValue(element, kAXRoleAttribute, &role);
    
    if (error == kAXErrorSuccess) {
        CFStringRef roleStr = static_cast<CFStringRef>(role);
        char buffer[256];
        if (CFStringGetCString(roleStr, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
            std::cout << "Element role: " << buffer << std::endl;
        }
        CFRelease(role);
    } else {
        std::cerr << "Failed to get role for element." << std::endl;
    }

    // Retrieve and print the title of the element, if available
    CFTypeRef title;
    error = AXUIElementCopyAttributeValue(element, kAXTitleAttribute, &title);
    if (error == kAXErrorSuccess && title) {
        CFStringRef titleStr = static_cast<CFStringRef>(title);
        char buffer[256];
        if (CFStringGetCString(titleStr, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
            std::cout << "Element title: " << buffer << std::endl;
        }
        CFRelease(title);
    } else {
        std::cout << "Element has no title or failed to get it." << std::endl;
    }
}


// Method to check if the element is focused
bool LiveInterface::isElementFocused(AXUIElementRef element) {
    CFTypeRef focused = NULL;
    AXUIElementCopyAttributeValue(element, kAXFocusedAttribute, &focused);

    if (focused == kCFBooleanTrue) {
        CFRelease(focused);
        return true;
    }

    if (focused) {
        CFRelease(focused);
    }
    return false;
}

// Method to focus the element
void LiveInterface::focusElement(AXUIElementRef element) {
    AXUIElementSetAttributeValue(element, kAXFocusedAttribute, kCFBooleanTrue);
}

// Method to set text in the element
void LiveInterface::setTextInElement(AXUIElementRef element, const char* text) {
    CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
    AXUIElementSetAttributeValue(element, kAXValueAttribute, cfText);
    CFRelease(cfText);
}

// Method to find and interact with the "Search, text field"
void LiveInterface::findAndInteractWithSearchField() {
    if (mainWindow_) {
        CFArrayRef children;
        AXUIElementCopyAttributeValue(mainWindow_, kAXChildrenAttribute, (CFTypeRef*)&children);

        for (CFIndex i = 0; i < CFArrayGetCount(children); i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
            
            CFStringRef role;
            AXUIElementCopyAttributeValue(child, kAXRoleAttribute, (CFTypeRef*)&role);
            
            if (CFStringCompare(role, kAXTextFieldRole, 0) == kCFCompareEqualTo) {
                CFStringRef description;
                AXUIElementCopyAttributeValue(child, kAXDescriptionAttribute, (CFTypeRef*)&description);
                
                if (CFStringCompare(description, CFSTR("Search"), 0) == kCFCompareEqualTo) {
                    printf("Found the Search, text field in app");

                    // Check if it's focused
                    if (!isElementFocused(child)) {
                        printf("Search field is not focused. Focusing now...\n");
                        focusElement(child);
                    } else {
                        printf("Search field is already focused.\n");
                    }

                    // Send text to the search field
                    setTextInElement(child, "Hello, world!");
                    printf("Text sent to search field.\n");
                }
                
                CFRelease(description);
            }
            CFRelease(role);
        }

        CFRelease(children);
    } else {
        printf("Failed to get main window for app");
    }
}

