#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

namespace AXAction {
    // Method to set text in the element
    inline void setTextInElement(AXUIElementRef element, const char* text) {
        CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
        AXUIElementSetAttributeValue(element, kAXValueAttribute, cfText);
        CFRelease(cfText);
    }

    // Method to focus the element
    inline void focusElement(AXUIElementRef element) {
        AXUIElementSetAttributeValue(element, kAXFocusedAttribute, kCFBooleanTrue);
        // TODO error checking
    }

    inline void closeSpecificWindow(WindowHandle windowHandle) {
        AXUIElementRef elem = static_cast<AXUIElementRef>(windowHandle);
        if (!elem) {
            return;
        }

        // Try to get the close button
        AXUIElementRef closeButton = nullptr;
        AXError error = AXUIElementCopyAttributeValue(elem, kAXCloseButtonAttribute, (CFTypeRef *)&closeButton);
        
        if (error != kAXErrorSuccess || !closeButton) {
            return;
        }

        // Press the close button
        error = AXUIElementPerformAction(closeButton, kAXPressAction);
        CFRelease(closeButton);

        if (error != kAXErrorSuccess) {
            return;
        }
    }
}
