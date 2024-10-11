#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

namespace AXWindow {
    inline bool isPluginWindow(AXUIElementRef element) {
        if (!element) {
            logger->debug("element is null");
            return false;
        }
        
        CFStringRef role;
        AXUIElementCopyAttributeValue(element, kAXRoleAttribute, (CFTypeRef*)&role);
        if (!role) {
            logger->debug("error getting role");
            return false;
        }
        
        // not a window
        if (! (CFStringCompare(role, kAXWindowRole, 0) == kCFCompareEqualTo)) {
            if (role) CFRelease(role);
            logger->debug("not an AXWindow");
            return false;
        }

        // not a floating window
        CFStringRef subrole;
        AXUIElementCopyAttributeValue(element, kAXSubroleAttribute, (CFTypeRef*)&subrole);
        if (CFStringCompare(subrole, kAXFloatingWindowSubrole, 0) == kCFCompareEqualTo) {
            if (role) CFRelease(role);
            if (subrole) CFRelease(subrole);
            return true;
        }
        logger->debug("not an AXFloatingWindow");

        return false;
    }

    inline void setWindowBounds(AXUIElementRef window, int x, int y, int width, int height) {
        CGPoint position = {static_cast<CGFloat>(x), static_cast<CGFloat>(y)};
        CGSize size = {static_cast<CGFloat>(width), static_cast<CGFloat>(height)};

        AXValueRef positionRef = AXValueCreate(static_cast<AXValueType>(kAXValueCGPointType), &position);
        AXUIElementSetAttributeValue(window, kAXPositionAttribute, positionRef);
        CFRelease(positionRef);

        AXValueRef sizeRef = AXValueCreate(static_cast<AXValueType>(kAXValueCGSizeType), &size);
        AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeRef);
        CFRelease(sizeRef);

        // Update the cached bounds
        cachedWindowBounds_[window] = CGRectMake(x, y, width, height);
    }

    inline CGRect getWindowBounds(AXUIElementRef window) {
        // Check if we have cached bounds for this window
        auto it = cachedWindowBounds_.find(window);
        if (it != cachedWindowBounds_.end()) {
            return it->second;
        }

        // If not cached, get the current bounds
        CGPoint position;
        CGSize size;
        AXValueRef positionRef, sizeRef;

        AXUIElementCopyAttributeValue(window, kAXPositionAttribute, (CFTypeRef*)&positionRef);
        AXUIElementCopyAttributeValue(window, kAXSizeAttribute, (CFTypeRef*)&sizeRef);

        AXValueGetValue(positionRef, static_cast<AXValueType>(kAXValueCGPointType), &position);
        AXValueGetValue(sizeRef, static_cast<AXValueType>(kAXValueCGSizeType), &size);

        CGRect bounds = CGRectMake(position.x, position.y, size.width, size.height);

        // Cache the bounds
        cachedWindowBounds_[window] = bounds;

        if (positionRef) CFRelease(positionRef);
        if (sizeRef) CFRelease(sizeRef);

        return bounds;
    }
}
