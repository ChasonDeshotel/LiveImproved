#ifndef LIVE_INTERFACE_H
#define LIVE_INTERFACE_H

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <vector>

class LiveInterface {
public:
    // Constructor
    LiveInterface();

    // Destructor
    ~LiveInterface();

    bool isAnyTextFieldFocused();

    // Method to check if the element is focused
    bool isElementFocused(AXUIElementRef element);

    // Method to focus the element
    void focusElement(AXUIElementRef element);

    // Method to set text in the element
    void setTextInElement(AXUIElementRef element, const char* text);

    // Method to find and interact with the "Search, text field"
    void findAndInteractWithSearchField();

    AXUIElementRef getMainWindow();
    bool isAnyTextFieldFocusedRecursive(AXUIElementRef element, int level);
    AXUIElementRef findElementByIdentifier(AXUIElementRef parent, CFStringRef identifier, int level);
    AXUIElementRef findElementByAttribute(AXUIElementRef parent, CFStringRef valueToFind, CFStringRef searchAttribute, int level);
    std::vector<AXUIElementRef> findElementsByType(AXUIElementRef parent, CFStringRef roleToFind, int level);
    void printAllAttributes(AXUIElementRef element);
    void searchFocusedTextField(AXUIElementRef element);
    void printFocusedElementInChildren(AXUIElementRef element);
    void printFocusedElementInfo(AXUIElementRef element);
    void printFocusedChildElementInfo(AXUIElementRef element);
    void printElementInfo(AXUIElementRef element);

private:
    AXUIElementRef findApplicationWindow();
    AXUIElementRef mainWindow_;
};

#endif
