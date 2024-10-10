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
    void printAllAttributeValues(AXUIElementRef element);
    void searchFocusedTextField(AXUIElementRef element);
    void printFocusedElementInChildren(AXUIElementRef element);
    void printFocusedElementInfo(AXUIElementRef element);
    void printFocusedChildElementInfo(AXUIElementRef element);
    void printElementInfo(AXUIElementRef element, std::string prefix);
    void printAXElementChildrenRecursively(AXUIElementRef element, int depth, int currentDepth);
    void setupPluginWindowChangeObserver(std::function<void()> callback) override;
    void removePluginWindowChangeObserver() override;
    bool isPluginWindow(AXUIElementRef element);
    CFArrayRef getAllWindows();
    AXUIElementRef getFrontmostWindow();
    void closeFocusedPluginWindow() override;
    void closeSpecificWindow(WindowHandle element) override;
    void tilePluginWindows();

private:
    std::function<std::shared_ptr<ILogHandler>()> logHandler_;
    std::function<std::shared_ptr<EventHandler>()> eventHandler_;
    void setWindowBounds(AXUIElementRef window, int x, int y, int width, int height);
    std::map<AXUIElementRef, CGRect> cachedWindowBounds_;
    CGRect getWindowBounds(AXUIElementRef window);

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
    AXUIElementRef getFocusedElement();


    bool isElementValid(AXUIElementRef element);

    std::function<void()> createCallback_;
    static void pluginWindowCreateCallback(AXObserverRef observer, AXUIElementRef element,
                                     CFStringRef notification, void* context);

    std::function<void()> destroyCallback_;
    static void pluginWindowDestroyCallback(AXObserverRef observer, AXUIElementRef element,
                                     CFStringRef notification, void* context);

    void tilePluginWindows() {
        std::vector<AXUIElementRef> pluginWindows = getPluginWindowsFromLiveAX(0);  // Get all plugin windows
        if (pluginWindows.empty()) return;

        // Get the screen size
        CGRect screenBounds = [[NSScreen mainScreen] frame];
        int screenWidth = screenBounds.size.width;
        int screenHeight = screenBounds.size.height;

        // Calculate the total area of all plugin windows
        double totalArea = 0;
        for (const auto& window : pluginWindows) {
            CGRect bounds = getWindowBounds(window);
            totalArea += bounds.size.width * bounds.size.height;
        }

        // Calculate the scaling factor to fit all windows on the screen
        double scaleFactor = std::sqrt((screenWidth * screenHeight) / totalArea);

        // Calculate the number of rows and columns
        int numWindows = pluginWindows.size();
        int numCols = std::ceil(std::sqrt(numWindows));
        int numRows = std::ceil(static_cast<double>(numWindows) / numCols);

        // Arrange windows
        int currentX = 0;
        int currentY = 0;
        int maxRowHeight = 0;

        for (int i = 0; i < numWindows; ++i) {
            CGRect originalBounds = getWindowBounds(pluginWindows[i]);
            int scaledWidth = std::round(originalBounds.size.width * scaleFactor);
            int scaledHeight = std::round(originalBounds.size.height * scaleFactor);

            if (currentX + scaledWidth > screenWidth) {
                currentX = 0;
                currentY += maxRowHeight;
                maxRowHeight = 0;
            }

            setWindowBounds(pluginWindows[i], currentX, currentY, scaledWidth, scaledHeight);

            currentX += scaledWidth;
            maxRowHeight = std::max(maxRowHeight, scaledHeight);
        }
    }

    void setWindowBounds(AXUIElementRef window, int x, int y, int width, int height) {
        CGPoint position = CGPointMake(x, y);
        CGSize size = CGSizeMake(width, height);

        AXValueRef positionRef = AXValueCreate(kAXValueCGPointType, &position);
        AXValueRef sizeRef = AXValueCreate(kAXValueCGSizeType, &size);

        AXUIElementSetAttributeValue(window, kAXPositionAttribute, positionRef);
        AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeRef);

        CFRelease(positionRef);
        CFRelease(sizeRef);

        // Update the cached bounds
        cachedWindowBounds_[window] = CGRectMake(x, y, width, height);
    }

    CGRect getWindowBounds(AXUIElementRef window) {
        // Check if we have cached bounds for this window
        auto it = cachedWindowBounds_.find(window);
        if (it != cachedWindowBounds_.end()) {
            return it->second;
        }

        // If not cached, get the current bounds
        CGPoint position;
        CGSize size;
        AXValueRef positionRef, sizeRef;

        AXUIElementCopyAttributeValue(window, kAXPositionAttribute, (CFTypeRef*)&positionRef);
        AXUIElementCopyAttributeValue(window, kAXSizeAttribute, (CFTypeRef*)&sizeRef);

        AXValueGetValue(positionRef, kAXValueCGPointType, &position);
        AXValueGetValue(sizeRef, kAXValueCGSizeType, &size);

        CFRelease(positionRef);
        CFRelease(sizeRef);

        CGRect bounds = CGRectMake(position.x, position.y, size.width, size.height);

        // Cache the bounds
        cachedWindowBounds_[window] = bounds;

        return bounds;
    }
};
