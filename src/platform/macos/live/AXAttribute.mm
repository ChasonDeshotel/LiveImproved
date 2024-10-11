#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include "LogGlobal.h"

#include "AXAction.h"
#include "AXAttribute.h"

namespace AXAttribute {
    bool isEnabled(AXUIElementRef elem) {
        CFBooleanRef enabled = nullptr;
        if (AXUIElementCopyAttributeValue(elem, kAXEnabledAttribute, (CFTypeRef*)&enabled) == kAXErrorSuccess && enabled) {
            bool isEnabled = CFBooleanGetValue(enabled);  // Convert CFBooleanRef to bool
            CFRelease(enabled);  // Release the CFBooleanRef after use
            return isEnabled;
        } else {
            std::cerr << "AXEnabled attribute not found or failed to retrieve." << std::endl;
            return false;  // Return false if the element doesn't have the AXEnabled attribute or retrieval failed
        }
    }

    bool isValid(AXUIElementRef element) {
        CFTypeRef role = nullptr;
        AXError result = AXUIElementCopyAttributeValue(element, kAXRoleAttribute, &role);
            
        if (result != kAXErrorSuccess || role == nullptr) {
            logger->warn("AXUIElementRef is invalid, failed to get role attribute!");
            return false;
        }

        CFRelease(role);
        return true;
    }

    // Method to check if the element is focused
    bool isFocused(AXUIElementRef element) {
        CFTypeRef isFocused = nullptr;
        AXError result = AXUIElementCopyAttributeValue(element, kAXFocusedAttribute, &isFocused);

        if (result == kAXErrorSuccess && isFocused == kCFBooleanTrue) {
            logger->info("element is focused.");
            if (isFocused != nullptr) {
                CFRelease(isFocused);
            }
            return true;
        }

        logger->info("element is not focused.");
        if (isFocused != nullptr) {
            CFRelease(isFocused);
        }
        return false;
    }

    CFStringRef getRole(AXUIElementRef elementRef) {
        CFStringRef role = nullptr;
        AXError error = AXUIElementCopyAttributeValue(elementRef, kAXRoleAttribute, (CFTypeRef *)&role);

        if (error != kAXErrorSuccess || !role) {
            std::cout << "Failed to get role for element. Error: " << error << std::endl;
            return nullptr;
        }

        return role;
    }
}
