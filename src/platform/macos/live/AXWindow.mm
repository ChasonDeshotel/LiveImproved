#import <ApplicationServices/ApplicationServices.h>

#include "LogGlobal.h"
#include "MacUtils.h"

#include "AXAttribute.h"

namespace AXWindow {
    bool isPluginWindow(AXUIElementRef element) {
        if (!AXAttribute::isValid(element)) {
            logger->debug("element is null");
            return false;
        }
        CFRetain(element);
        
        CFStringRef role = nullptr;
        AXError error = AXUIElementCopyAttributeValue(element, kAXRoleAttribute, castutil::toCFTypeRef(&role));
        if (error != kAXErrorSuccess || !role) {
            logger->warn("error getting role");
            return false;
        }
        
        if (! (CFStringCompare(role, kAXWindowRole, 0) == kCFCompareEqualTo)) {
            if (role) CFRelease(role);
            logger->debug("not an AXWindow");
            return false;
        }

        CFStringRef subrole = nullptr;
        error = AXUIElementCopyAttributeValue(element, kAXSubroleAttribute, castutil::toCFTypeRef(&subrole));
        if (error != kAXErrorSuccess || !subrole) {
            logger->warn("error getting subrole");
            return false;
        }
        if (CFStringCompare(subrole, kAXFloatingWindowSubrole, 0) == kCFCompareEqualTo) {
            if (role) CFRelease(role);
            if (subrole) CFRelease(subrole);
            return true;
        }
        logger->debug("not an AXFloatingWindow");

        CFRelease(element);
        return false;
    }

    AXError setBounds(AXUIElementRef window, int x, int y, int width, int height) {
        if (!AXAttribute::isValid(window)) {
            logger->error("window is not valid");
            return kAXErrorFailure;
        }
        CFRetain(window);

        CGPoint position = {static_cast<CGFloat>(x), static_cast<CGFloat>(y)};
//        CGSize size = {static_cast<CGFloat>(width), static_cast<CGFloat>(height)};

        AXValueRef positionRef = AXValueCreate(static_cast<AXValueType>(kAXValueCGPointType), &position);
        AXError posError = AXUIElementSetAttributeValue(window, kAXPositionAttribute, positionRef);
        CFRelease(positionRef);

//        AXValueRef sizeRef = AXValueCreate(static_cast<AXValueType>(kAXValueCGSizeType), &size);
//        AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeRef);
//        CFRelease(sizeRef);
//
        logger->debug("set bounds to: (" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(width)
                        + "," + std::to_string(height));

        if (posError != kAXErrorSuccess) {
            return posError;
        }
        CFRelease(window);
        return posError;
        // Update the cached bounds
        //cachedWindowBounds_[window] = CGRectMake(x, y, width, height);
    }

    std::string rectToString(const CGRect& rect) {
        return "CGRect {x: " + std::to_string(rect.origin.x) +
               ", y: " + std::to_string(rect.origin.y) +
               ", width: " + std::to_string(rect.size.width) +
               ", height: " + std::to_string(rect.size.height) + "}";
    }

    CGRect getBounds(AXUIElementRef window) {
        CGRect rect;
        if (!AXAttribute::isValid(window)) {
            logger->error("window is not valid");
            rect = CGRectNull;
        }
        CFRetain(window);

        CGPoint position;
        CGSize size;
        AXValueRef positionRef = nullptr, sizeRef = nullptr;

        AXUIElementCopyAttributeValue(window, kAXPositionAttribute, castutil::toCFTypeRef(&positionRef));
        AXUIElementCopyAttributeValue(window, kAXSizeAttribute, castutil::toCFTypeRef(&sizeRef));

        AXValueGetValue(positionRef, static_cast<AXValueType>(kAXValueCGPointType), &position);
        AXValueGetValue(sizeRef, static_cast<AXValueType>(kAXValueCGSizeType), &size);

        CGRect bounds = CGRectMake(position.x, position.y, size.width, size.height);
        logger->info(rectToString(bounds));

        if (positionRef) CFRelease(positionRef);
        if (sizeRef) CFRelease(sizeRef);
        
        CFRelease(window);
        return bounds;
    }
}
