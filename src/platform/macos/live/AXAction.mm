#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

namespace AXAction {
    // Method to set text in the element
    void setTextInElement(AXUIElementRef element, const char* text) {
        CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
        AXUIElementSetAttributeValue(element, kAXValueAttribute, cfText);
        CFRelease(cfText);
    }

    // Method to focus the element
    void focusElement(AXUIElementRef element) {
        AXUIElementSetAttributeValue(element, kAXFocusedAttribute, kCFBooleanTrue);
        // TODO error checking
    }

    void closeSpecificWindow(void* windowHandle) {
        AXUIElementRef element = static_cast<AXUIElementRef>(windowHandle);
        if (!element) {
            return;
        }

        // Try to get the close button
        AXUIElementRef closeButton = nullptr;
        AXError error = AXUIElementCopyAttributeValue(element, kAXCloseButtonAttribute, (CFTypeRef *)&closeButton);
        
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
