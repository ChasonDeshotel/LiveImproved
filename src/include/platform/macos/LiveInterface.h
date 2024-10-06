#pragma once

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <functional>
#include <vector>

#include "ILiveInterface.h"

class ILogHandler;

class LiveInterface : public ILiveInterface {
public:
    // Constructor
    LiveInterface(std::function<std::shared_ptr<ILogHandler>()> logHandler);

    // Destructor
    ~LiveInterface() override;

    bool isAnyTextFieldFocused();

    // Method to check if the element is focused
    bool isElementFocused(AXUIElementRef element);

    // Method to focus the element
    void focusElement(AXUIElementRef element);
    int getMostRecentFloatingWindow() override;
    bool focusWindow(int windowID) override;
    CFArrayRef getAllWindows();
    AXUIElementRef getAppElement();
    CFStringRef getRole(AXUIElementRef elementRef);
    CFStringRef getSubrole(AXUIElementRef elementRef);

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
    std::function<std::shared_ptr<ILogHandler>()> logHandler_;

    AXUIElementRef findApplicationWindow();
    AXUIElementRef mainWindow_;
};
