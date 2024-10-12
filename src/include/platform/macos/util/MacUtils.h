#pragma once

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include <iostream>
#include <ostream>
#include <string>

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef element, CGWindowID* windowID);

namespace CFStringUtil {
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
