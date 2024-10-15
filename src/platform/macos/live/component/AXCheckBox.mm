#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#import "LogGlobal.h"
#import "MacUtils.h"

#import "AXAttribute.h"
#import "AXCheckBox.h"

namespace AXCheckBox {
    bool isChecked(AXUIElementRef checkbox) {
        if (!AXAttribute::isValid(checkbox)) {
            logger->warn("checkbox is invalid");
            return false;
        }
        CFBooleanRef value = nullptr;
        if (AXUIElementCopyAttributeValue(checkbox, kAXValueAttribute, castutil::toCFTypeRef(&value)) == kAXErrorSuccess && value) {
            bool isChecked = CFBooleanGetValue(value);
            CFRelease(value);
            return isChecked;
        } else {
            std::cerr << "AXValue attribute not found or failed to retrieve for checkbox." << std::endl;
            return false; // not found or false
        }
    }

    bool toggle(AXUIElementRef checkbox) {
        if (!AXAttribute::isValid(checkbox)) {
            logger->warn("checkbox is invalid");
            return false;
        }
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

    // returns whether it changed or not
    // -- not the state of isChecked
    bool toggleOn(AXUIElementRef checkbox) {
        if (!AXAttribute::isValid(checkbox)) {
            logger->warn("checkbox is invalid");
            return false;
        }
        if (isChecked(checkbox)) {
            logger->warn("already on");
            return false;
        }
        AXError error = AXUIElementPerformAction(checkbox, kAXPressAction);
        if (error == kAXErrorSuccess) {
            logger->info("Successfully pressed the checkbox.");
            return true;
        } else {
            logger->error("Failed to press the checkbox. Error: " + axerror::toString(error));
            return false;
        }
        return false;
    }

    // for closing and re-opening opened plugin windows
    // so that the plugin window order is correct in
    // the main window's AXChildren
    bool toggleOffOn(AXUIElementRef checkbox) {
        if (!AXAttribute::isValid(checkbox)) {
            logger->warn("checkbox is invalid");
            return false;
        }
        if (isChecked(checkbox)) {
            bool offPress = toggle(checkbox);
            usleep(10000);
            bool onPress = toggle(checkbox);
            if (onPress && offPress) {
                return true;
            }
        }
        return false;
    }
}
