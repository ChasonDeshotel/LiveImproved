#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>

//To determine if an AXTextField (or any accessibility element) is in edit mode (focused for text input) using macOS's Accessibility API, you can query the AXFocused and AXSelectedTextRange attributes via the AXUIElement API. Here's how you can do this:
//Steps:
//
//    Check AXFocused: The AXFocused attribute tells whether the AXTextField has the keyboard focus. If this is true, the text field is focused, and the user can interact with it.
//
//    Check AXSelectedTextRange: The AXSelectedTextRange attribute provides the currently selected range of text in the field. If this attribute is present and valid, it means the text field is in edit mode.

// Function to get the application's main window by its PID
AXUIElementRef getApplicationWindowByPID(pid_t pid) {
    AXUIElementRef appElement = AXUIElementCreateApplication(pid);
    
    AXUIElementRef mainWindow = nullptr;
    if (appElement) {
        // Get the main window of the application
        AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute, (CFTypeRef*)&mainWindow);
        CFRelease(appElement);
    }
    return mainWindow;
}

// Check if the element is focused
bool isElementFocused(AXUIElementRef element) {
    CFTypeRef focused = NULL;
    AXUIElementCopyAttributeValue(element, kAXFocusedAttribute, &focused);

    if (focused == kCFBooleanTrue) {
        CFRelease(focused);
        return true;
    }

    if (focused) {
        CFRelease(focused);
    }
    return false;
}

// Focus the element
void focusElement(AXUIElementRef element) {
    AXUIElementSetAttributeValue(element, kAXFocusedAttribute, kCFBooleanTrue);
}

// Set text in the element
void setTextInElement(AXUIElementRef element, const char* text) {
    CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
    AXUIElementSetAttributeValue(element, kAXValueAttribute, cfText);
    CFRelease(cfText);
}

// Find and interact with the "Search, text field"
void findAndInteractWithSearchFieldInApp(pid_t pid) {
    AXUIElementRef mainWindow = getApplicationWindowByPID(pid);
    
    if (mainWindow) {
        CFArrayRef children;
        AXUIElementCopyAttributeValue(mainWindow, kAXChildrenAttribute, (CFTypeRef*)&children);

        for (CFIndex i = 0; i < CFArrayGetCount(children); i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
            
            CFStringRef role;
            AXUIElementCopyAttributeValue(child, kAXRoleAttribute, (CFTypeRef*)&role);
            
            if (CFStringCompare(role, kAXTextFieldRole, 0) == kCFCompareEqualTo) {
                CFStringRef description;
                AXUIElementCopyAttributeValue(child, kAXDescriptionAttribute, (CFTypeRef*)&description);
                
                if (CFStringCompare(description, CFSTR("Search"), 0) == kCFCompareEqualTo) {
                    printf("Found the Search, text field in app with PID %d.\n", pid);

                    // Check if it's focused
                    if (!isElementFocused(child)) {
                        printf("Search field is not focused. Focusing now...\n");
                        focusElement(child);
                    } else {
                        printf("Search field is already focused.\n");
                    }

                    // Send text to the search field
                    setTextInElement(child, "Hello, world!");
                    printf("Text sent to search field.\n");
                }
                
                CFRelease(description);
            }
            CFRelease(role);
        }

        CFRelease(children);
        CFRelease(mainWindow);
    } else {
        printf("Failed to get main window for app with PID %d.\n", pid);
    }
}
