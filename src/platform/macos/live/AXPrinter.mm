#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include <string>

#include "MacUtils.h"
#include "AXAttribute.h"
#include "AXPrinter.h"

namespace AXPrinter {
    void printElementInfo(AXUIElementRef element, std::string prefix) {
        CFStringRef roleStr = nullptr, subrole = nullptr, title = nullptr, identifier = nullptr;
        char buffer[256];

        if (!AXAttribute::isValid(element)) return;
        
        CGWindowID windowID = 0;
        AXError error = _AXUIElementGetWindow(element, &windowID);
        if (error == kAXErrorSuccess && windowID > 0) {
            prefix += "ID: " + std::to_string(static_cast<int>(windowID));
        } else {
            logger->warn("unable to get window");
            return;
        }

    //        std::cout << prefix << "ID: " << windowID << std::endl;
    //    if (AXUIElementCopyAttributeValue(element, kAXRoleAttribute, (CFTypeRef*)&role) == kAXErrorSuccess) {
    //        CFStringGetCString(role, buffer, sizeof(buffer), kCFStringEncodingUTF8);
    //        std::cout << prefix << "Role: " << buffer << std::endl;
    //        CFRelease(role);
    //    }
    //
    //    if (AXUIElementCopyAttributeValue(element, kAXSubroleAttribute, (CFTypeRef*)&subrole) == kAXErrorSuccess) {
    //        CFStringGetCString(subrole, buffer, sizeof(buffer), kCFStringEncodingUTF8);
    //        std::cout << prefix << "Subrole: " << buffer << std::endl;
    //        CFRelease(subrole);
    //    }

        if (AXUIElementCopyAttributeValue(element, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess) {
            CFStringGetCString(title, buffer, sizeof(buffer), kCFStringEncodingUTF8);
            std::cout << prefix << "  Title: " << buffer << std::endl;
            CFRelease(title);
        }

    //    if (AXUIElementCopyAttributeValue(element, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess) {
    //        CFStringGetCString(identifier, buffer, sizeof(buffer), kCFStringEncodingUTF8);
    //        std::cout << prefix << "Identifier (Class): " << buffer << std::endl;
    //        CFRelease(identifier);
    //    }
        CFTypeRef attributeValue = nullptr;
        const char* cString = "AXFocused";
        CFStringRef attributeName = CFStringCreateWithCString(NULL, cString, kCFStringEncodingUTF8);

        error = AXUIElementCopyAttributeValue(element, attributeName, &attributeValue);
        CFRelease(attributeName);
        if (error == kAXErrorSuccess && attributeValue != nullptr) {
            if (CFGetTypeID(attributeValue) == CFBooleanGetTypeID()) {
                // If the value is a boolean
                Boolean boolValue = CFBooleanGetValue((CFBooleanRef)attributeValue);
                std::cout << prefix << "AXFocused: " << (boolValue ? "true" : "false") << std::endl;
            }
        }
        CFRelease(element);
    }

    void printAXElementChildrenRecursively(AXUIElementRef element, int depth, int currentDepth) {
        if (currentDepth >= depth) {
            return; // Stop recursion when reaching the depth limit
        }

        CFTypeRef childrenValue = nullptr;
        AXError error = AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, &childrenValue);

        if (error == kAXErrorSuccess && childrenValue != nullptr) {
            if (CFGetTypeID(childrenValue) == CFArrayGetTypeID()) {
                CFArrayRef children = (CFArrayRef)childrenValue;
                CFIndex childCount = CFArrayGetCount(children);

                for (CFIndex i = 0; i < childCount; i++) {
                    AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);

                    // Call printAllAttributeValues on each child
                    printAllAttributeValues(child);

                    // Recursively go through the child's children
                    printAXElementChildrenRecursively(child, depth, currentDepth + 1);
                }
            }
            CFRelease(childrenValue); // Don't forget to release when done
        }
    }

    void printAXTree(AXUIElementRef element, int level) {
        CFArrayRef children = nullptr;
        AXError error = AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, (CFTypeRef*)&children);

        if (error == kAXErrorSuccess && children) {
            CFIndex count = CFArrayGetCount(children);
            for (CFIndex i = 0; i < count; ++i) {
                AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);

                // Get and print the title of each child (if available)
                CFStringRef title = nullptr;
                if (AXUIElementCopyAttributeValue(child, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess && title) {
                    std::cout << std::string(level * 2, ' ') << "Title: ";
                    cfstringutil::printCFString(title);
                    CFRelease(title);
                } else {
                    std::cout << std::string(level * 2, ' ') << "Title: (none)" << std::endl;
                }

                // Get and print the identifier of each child (if available)
                CFStringRef identifier = nullptr;
                if (AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
                    std::cout << std::string(level * 2, ' ') << "Identifier: ";
                    cfstringutil::printCFString(identifier);
                    CFRelease(identifier);
                } else {
                    std::cout << std::string(level * 2, ' ') << "Identifier: (none)" << std::endl;
                }

                // Recursively print the tree
                printAXTree(child, level + 1);
            }
            CFRelease(children);
        } else {
            std::cerr << std::string(level * 2, ' ') << "Failed to retrieve children or no children." << std::endl;
        }
    }

    void printAXTitle(AXUIElementRef elem) {
        return; // TODO
        CFStringRef title = nullptr;
        if (AXUIElementCopyAttributeValue(elem, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess && title) {
            std::cout << " Title: ";
            cfstringutil::printCFString(title);
            CFRelease(title);
        } else {
            std::cout << " Title: (none)" << std::endl;
        }
    }

    void printAXIdentifier(AXUIElementRef elem) {
        CFStringRef identifier = nullptr;
        if (AXUIElementCopyAttributeValue(elem, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
            std::cout << " Identifier: ";
            cfstringutil::printCFString(identifier);
            CFRelease(identifier);
        } else {
            std::cout << " Identifier: (none)" << std::endl;
        }
    }

    #ifndef kAXChildrenInNavigationOrderAttribute
    #define kAXChildrenInNavigationOrderAttribute CFSTR("AXChildrenInNavigationOrder")
    #endif

    void printAXChildrenInNavigationOrder(AXUIElementRef element) {
        if (!element) {
            std::cerr << "Error: AXUIElementRef is null." << std::endl;
            return;
        }

        CFArrayRef navigationChildren = nullptr;
        AXError error = AXUIElementCopyAttributeValue(element, kAXChildrenInNavigationOrderAttribute, (CFTypeRef*)&navigationChildren);

        if (error == kAXErrorSuccess && navigationChildren) {
            CFIndex count = CFArrayGetCount(navigationChildren);
            std::cout << "Number of children in navigation order: " << count << std::endl;

            for (CFIndex i = 0; i < count; i++) {
                AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(navigationChildren, i);

                // Retrieve and print the identifier of each child
                CFStringRef identifier = nullptr;
                if (AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
                    std::cout << " Child " << i << " Identifier: ";
                    cfstringutil::printCFString(identifier);  // Helper function to print CFStringRef
                    CFRelease(identifier);
                } else {
                    std::cout << " Child " << i << " Identifier: (none)" << std::endl;
                }

                // Optionally, you could print other attributes, like title or role
            }

            CFRelease(navigationChildren);  // Release the array after use
        } else {
            std::cerr << "Failed to retrieve AXChildrenInNavigationOrder or no children found." << std::endl;
        }
    }


    void printFocusedElementInChildren(AXUIElementRef parent) {
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

    void printChildren(AXUIElementRef element, int level) {
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

    void printFocusedChildElementInfo(const AXUIElementRef mainWindow) {
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


    void printAllAttributeValues(AXUIElementRef element) {
        CFArrayRef attributeNames = nullptr;
        AXError error = AXUIElementCopyAttributeNames(element, &attributeNames);

        // Handle errors in copying attribute names
        if (error != kAXErrorSuccess || !attributeNames) {
            logger->debug("Failed to retrieve attribute names for the element.");
            return;
        }

        CFIndex count = CFArrayGetCount(attributeNames);
        for (CFIndex i = 0; i < count; i++) {
            CFStringRef attributeName = (CFStringRef)CFArrayGetValueAtIndex(attributeNames, i);
            char nameBuffer[256];
            CFStringGetCString(attributeName, nameBuffer, sizeof(nameBuffer), kCFStringEncodingUTF8);
            logger->debug("Attribute: " + std::string(nameBuffer));

            // Get the attribute value
            CFTypeRef attributeValue = nullptr;
            AXError valueError = AXUIElementCopyAttributeValue(element, attributeName, &attributeValue);
            if (valueError == kAXErrorSuccess && attributeValue != nullptr) {
                // Handle different types of attribute values
                if (CFGetTypeID(attributeValue) == CFStringGetTypeID()) {
                    char valueBuffer[256];
                    CFStringGetCString((CFStringRef)attributeValue, valueBuffer, sizeof(valueBuffer), kCFStringEncodingUTF8);
                    logger->debug("  Value (string): " + std::string(valueBuffer));
                } else if (CFGetTypeID(attributeValue) == CFNumberGetTypeID()) {
                    int intValue;
                    CFNumberGetValue((CFNumberRef)attributeValue, kCFNumberIntType, &intValue);
                    logger->debug("  Value (number): " + std::to_string(intValue));
                } else if (CFGetTypeID(attributeValue) == CFBooleanGetTypeID()) {
                    Boolean boolValue = CFBooleanGetValue((CFBooleanRef)attributeValue);
                    logger->debug("  Value (boolean): " + std::string(boolValue ? "true" : "false"));
                } else if (CFGetTypeID(attributeValue) == CFArrayGetTypeID()) {
                    CFArrayRef arrayValue = (CFArrayRef)attributeValue;
                    CFIndex arrayCount = CFArrayGetCount(arrayValue);
                    logger->debug("  Value (array), size: " + std::to_string(arrayCount));
                    for (CFIndex j = 0; j < arrayCount; j++) {
                        CFTypeRef arrayElement = CFArrayGetValueAtIndex(arrayValue, j);
                        // You can recursively print array elements based on type here, if needed
                    }
                } else if (CFGetTypeID(attributeValue) == AXValueGetTypeID()) {
                   // AXValueType valueType = AXValueGetType((AXValueRef)attributeValue);
                   // if (valueType == kAXValueCGPointType) {
                   //     CGPoint point;
                   //     AXValueGetValue((AXValueRef)attributeValue, kAXValueCGPointType, &point);
                   //     logger->debug("  Value (CGPoint): (" + std::to_string(point.x) + ", " + std::to_string(point.y) + ")");
                   // } else if (valueType == kAXValueCGSizeType) {
                   //     CGSize size;
                   //     AXValueGetValue((AXValueRef)attributeValue, kAXValueCGSizeType, &size);
                   //     logger->debug("  Value (CGSize): (" + std::to_string(size.width) + ", " + std::to_string(size.height) + ")");
                   // } else {
                   //     logger->debug("  Value (AXValue of unknown type)");
                   // }
                } else {
                    // Handle other types or unknown types
                    logger->debug("  Value (unknown type)");
                }

                // Release the attribute value after use
                CFRelease(attributeValue);
            } else {
                logger->debug("  Failed to get value for this attribute");
            }
        }

        // Release the attribute names array
        CFRelease(attributeNames);
    }

    void printAllAttributes(const AXUIElementRef element) {
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
}
