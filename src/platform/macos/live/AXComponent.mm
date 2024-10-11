#pragma once

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#import "LogGlobal.h"

namespace AXCheckBox {
    inline bool isChecked(AXUIElementRef elem) {
        CFBooleanRef value = nullptr;
        if (AXUIElementCopyAttributeValue(elem, kAXValueAttribute, (CFTypeRef*)&value) == kAXErrorSuccess && value) {
            bool isChecked = CFBooleanGetValue(value);
            CFRelease(value);
            return isChecked;
        } else {
            std::cerr << "AXValue attribute not found or failed to retrieve for checkbox." << std::endl;
            return false; // not found or false
        }
    }

    inline bool toggle(AXUIElementRef checkbox) {
        if (!checkbox) {
            std::cerr << "Error: AXUIElementRef is null." << std::endl;
            return false;
        }

        AXError error = AXUIElementPerformAction(checkbox, kAXPressAction);

        if (error == kAXErrorSuccess) {
            logger->info("Successfully pressed the checkbox.");
            return true;
        } else {
            std::cerr << "Failed to press the checkbox. Error: " << error << std::endl;
            return false;
        }
    }

    // for closing and re-opening opened plugin windows
    // so that the plugin window order is correct in
    // the main window's AXChildren
    inline bool toggleOffOn(AXUIElementRef checkbox) {
        if (!isChecked(checkbox)) {
            logger->warn("not checked - already on");
            return false;
        }
        if (isChecked(checkbox)) {
            bool offPress = toggle(checkbox);
            usleep(20000);
            bool onPress = toggle(checkbox);
            if (onPress && offPress) {
                return true;
            }
        }
        return false;
    }
}
