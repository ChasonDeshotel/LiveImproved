#pragma once

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>  // Cocoa for Objective-C code (NSView, NSWindow)
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>
#endif

#include <functional>
#include <map>
#include <set>
#include <vector>

#include "ILiveInterface.h"

class ILogHandler;
class EventHandler;

class LiveInterface : public ILiveInterface {
public:
    // Constructor
    LiveInterface(
        std::function<std::shared_ptr<ILogHandler>()> logHandler
        , std::function<std::shared_ptr<EventHandler>()> eventHandler
    );

    // Destructor
    ~LiveInterface() override;

    std::vector<AXUIElementRef> pluginWindows_;

    bool isAnyTextFieldFocused();

    // Method to check if the element is focused
    bool isElementFocused(AXUIElementRef element);

    // Method to focus the element
    void focusElement(AXUIElementRef element);
    void safelyFocusAfterDestroy(int windowIDToDestroy, int windowIDToFocus);

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
    void printElementInfo(AXUIElementRef element, std::string prefix);
    void setupPluginWindowChangeObserver(std::function<void()> callback) override;
    void removePluginWindowChangeObserver() override;
    bool isPluginWindow(AXUIElementRef element);
    void closeFocusedPluginWindow() override;
    void closeSpecificWindow(WindowHandle element) override;

private:
    std::function<std::shared_ptr<ILogHandler>()> logHandler_;
    std::function<std::shared_ptr<EventHandler>()> eventHandler_;

    bool windowCloseInProgress_ = false;
    AXUIElementRef findApplicationWindow();
    AXUIElementRef mainWindow_;
    AXUIElementRef appElement_;
    AXObserverRef pluginWindowCreateObserver_;
    AXObserverRef pluginWindowDestroyObserver_;

    std::vector<AXUIElementRef> getPluginWindowsFromLiveAX(int limit);
    std::vector<AXUIElementRef> getCurrentPluginWindows();
    bool isAnyPluginWindowFocused();
    AXUIElementRef getFocusedPluginWindow();

    bool isElementValid(AXUIElementRef element);

    std::function<void()> createCallback_;
    static void pluginWindowCreateCallback(AXObserverRef observer, AXUIElementRef element,
                                     CFStringRef notification, void* context);

    std::function<void()> destroyCallback_;
    static void pluginWindowDestroyCallback(AXObserverRef observer, AXUIElementRef element,
                                     CFStringRef notification, void* context);
};
