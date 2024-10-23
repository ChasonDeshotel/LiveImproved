#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include "LogGlobal.h"
#include "MacUtils.h"

#include "AXAttribute.h"

namespace AXAttribute {
    bool isEnabled(const AXUIElementRef elem) {
        CFBooleanRef enabled = nullptr;
        if (AXUIElementCopyAttributeValue(elem, kAXEnabledAttribute, castutil::toCFTypeRef(&enabled)) == kAXErrorSuccess && enabled) {
            bool isEnabled = CFBooleanGetValue(enabled);  // Convert CFBooleanRef to bool
            CFRelease(enabled);  // Release the CFBooleanRef after use
            return isEnabled;
        } else {
            //logger->error("AXEnabled attribute not found or failed to retrieve.");
            return false;  // Return false if the element doesn't have the AXEnabled attribute or retrieval failed
        }
    }

    bool isValid(const AXUIElementRef element) {
        CFTypeRef role = nullptr;
        AXError result = AXUIElementCopyAttributeValue(element, kAXRoleAttribute, &role);
            
        if (result != kAXErrorSuccess || role == nullptr) {
            //logger->warn("AXUIElementRef is invalid, failed to get role attribute!");
            return false;
        }

        CFRelease(role);
        return true;
    }

    // Method to check if the element is focused
    bool isFocused(const AXUIElementRef element) {
        CFTypeRef isFocused = nullptr;
        AXError result = AXUIElementCopyAttributeValue(element, kAXFocusedAttribute, &isFocused);

        if (result != kAXErrorSuccess) {
            return false;
        }

        if (isFocused == kCFBooleanTrue) {
            CFRelease(isFocused);
            return true;
        }

        CFRelease(isFocused);
        return false;
    }

    CFStringRef getRole(const AXUIElementRef elementRef) {
        CFStringRef role = nullptr;
        AXError error = AXUIElementCopyAttributeValue(elementRef, kAXRoleAttribute, castutil::toCFTypeRef(&role));

        if (error != kAXErrorSuccess || !role) {
            //logger->warn("Failed to get role for element. Error: " + axerror::toString(error));
            return nullptr;
        }

        return role;
    }
}
