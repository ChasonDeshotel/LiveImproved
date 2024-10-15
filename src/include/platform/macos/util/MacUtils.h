#pragma once

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include <iostream>
#include <ostream>
#include <string>

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef element, CGWindowID* windowID);

namespace cfstringutil {
    inline std::string getStringFromCFString(CFStringRef cfString) {
        if (!cfString) return "";

        CFIndex length = CFStringGetLength(cfString);
        CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        std::vector<char> buffer(maxSize);

        if (CFStringGetCString(cfString, buffer.data(), maxSize, kCFStringEncodingUTF8)) {
            return std::string(buffer.data());
        }
        return "";
    }

    // Helper function to print CFString safely
    inline void printCFString(CFStringRef str) {
        if (str == nullptr) return;

        const int bufferSize = 256;
        char buffer[bufferSize];
        if (CFStringGetCString(str, buffer, bufferSize, kCFStringEncodingUTF8)) {
            std::cout << buffer << std::endl;
        } else {
            std::cerr << "Unable to retrieve CFString value." << std::endl;
        }
    }
}

namespace axerror {
    inline auto toString(AXError error) -> std::string {
        switch (error) {
            case kAXErrorSuccess:
                return "kAXErrorSuccess";
            case kAXErrorFailure:
                return "kAXErrorFailure";
            case kAXErrorIllegalArgument:
                return "kAXErrorIllegalArgument";
            case kAXErrorInvalidUIElement:
                return "kAXErrorInvalidUIElement";
            case kAXErrorInvalidUIElementObserver:
                return "kAXErrorInvalidUIElementObserver";
            case kAXErrorCannotComplete:
                return "kAXErrorCannotComplete";
            case kAXErrorAttributeUnsupported:
                return "kAXErrorAttributeUnsupported";
            case kAXErrorActionUnsupported:
                return "kAXErrorActionUnsupported";
            case kAXErrorNotificationUnsupported:
                return "kAXErrorNotificationUnsupported";
            case kAXErrorNotImplemented:
                return "kAXErrorNotImplemented";
            case kAXErrorNotificationAlreadyRegistered:
                return "kAXErrorNotificationAlreadyRegistered";
            case kAXErrorNotificationNotRegistered:
                return "kAXErrorNotificationNotRegistered";
            case kAXErrorAPIDisabled:
                return "kAXErrorAPIDisabled";
            case kAXErrorNoValue:
                return "kAXErrorNoValue";
            case kAXErrorParameterizedAttributeUnsupported:
                return "kAXErrorParameterizedAttributeUnsupported";
            case kAXErrorNotEnoughPrecision:
                return "kAXErrorNotEnoughPrecision";
            default:
                return "Unknown AXError";
        }
    }
}

namespace castutil {
    inline auto toCFTypeRef(void* value) -> CFTypeRef* {
        // NOLINTNEXTLINE - must cast between obj-c and c++ types
        return static_cast<CFTypeRef*>(value);
    }

    inline auto toAXUIElementRef(const void* value) -> AXUIElementRef {
        // NOLINTNEXTLINE - must cast between obj-c and c++ types
        return static_cast<AXUIElementRef>(value);
    }
}
