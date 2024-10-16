#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include <objc/runtime.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include "LogGlobal.h"
#include "MacUtils.h"

#include "IEventHandler.h"

#include "AXAttribute.h"
#include "AXComponents.h"
#include "AXFinder.h"
#include "AXInteraction.h"
#include "AXPrinter.h"
#include "AXWindow.h"

#include "LiveInterface.h"
#include "PID.h"

LiveInterface::LiveInterface(std::function<std::shared_ptr<IEventHandler>()> eventHandler)
    : ILiveInterface()
    , eventHandler_(std::move(eventHandler))
    , pluginWindowCreateObserver_()
    , pluginWindowDestroyObserver_()
{
    setupPluginWindowChangeObserver([]() {
        // logger->info("Window change detected!");
    });
}

LiveInterface::~LiveInterface() {}

void LiveInterface::setupPluginWindowChangeObserver(std::function<void()> callback) {
    pid_t livePID = PID::getInstance().livePID();
    if (livePID == -1) {
        logger->error("Live is not running");
        return;
    }

    logger->debug("set up plugin window change observer");

    AXUIElementRef appElement = AXFinder::appElement();
    if (!AXAttribute::isValid(appElement)) {
        logger->error("unable to get app element");
        return;
    }
    
    // create the window create observer, set the callback
    AXError error = AXObserverCreate(livePID, pluginWindowCreateCallback, &pluginWindowCreateObserver_);
    if (error != kAXErrorSuccess) {
        logger->error("Failed to create accessibility observer. Error: " + axerror::toString(error));
        return;
    }

    // same for destroy
    error = AXObserverCreate(livePID, pluginWindowDestroyCallback, &pluginWindowDestroyObserver_);
    if (error != kAXErrorSuccess) {
        logger->error("Failed to create accessibility observer. Error: " + axerror::toString(error));
        return;
    }

    error = AXObserverAddNotification(pluginWindowCreateObserver_, appElement, kAXCreatedNotification, this);
    if (error != kAXErrorSuccess) {
        logger->error("Failed to add creation notification. Error: " + axerror::toString(error));
        return;
    }

    // there is no window destroy observer, we'll have to check the type of what was destroyed
    error = AXObserverAddNotification(pluginWindowDestroyObserver_, appElement, kAXUIElementDestroyedNotification, this);
    if (error != kAXErrorSuccess) {
        logger->error("Failed to add destruction notification. Error: " + axerror::toString(error));
        return;
    }

    // Start the observers
    CFRunLoopAddSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(pluginWindowCreateObserver_), kCFRunLoopDefaultMode);
    CFRunLoopAddSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(pluginWindowDestroyObserver_), kCFRunLoopDefaultMode);

    createCallback_ = callback;
    destroyCallback_ = callback;
}

void LiveInterface::removePluginWindowChangeObserver() {
//    if (observer_ && mainWindow_) {
//        AXObserverRemoveNotification(observer_, mainWindow_, kAXCreatedNotification);
//        AXObserverRemoveNotification(observer_, mainWindow_, kAXUIElementDestroyedNotification);
//        CFRunLoopRemoveSource(CFRunLoopGetMain(), AXObserverGetRunLoopSource(observer_), kCFRunLoopDefaultMode);
//        CFRelease(observer_);
//        observer_ = nullptr;
//    }
//    if (mainWindow_) {
//        CFRelease(mainWindow_);
//        mainWindow_ = nullptr;
//    }
}

void LiveInterface::pluginWindowCreateCallback(AXObserverRef observer, AXUIElementRef element, 
                                         CFStringRef notification, void* context) {
    auto* interface = static_cast<LiveInterface*>(context);
    logger->info("create callback called");

    if (!AXWindow::isPluginWindow(element)) return;

    AXError error = AXUIElementPerformAction(element, kAXRaiseAction);
    if (error != kAXErrorSuccess) {
        logger->error("Failed to raise window. AXError: " + std::to_string(error));
    }

    if (interface && interface->createCallback_) {
        interface->createCallback_();
    }
}

void LiveInterface::pluginWindowDestroyCallback(AXObserverRef observer, AXUIElementRef element, 
                                         CFStringRef notification, void* context) {

    // element is invalid/empty at this point in the lifecycle -- it's just empty
    //interface->printElementInfo(element, "element from destroy: ");

    auto* interface = static_cast<LiveInterface*>(context);
    logger->debug("Destroy callback called");
//    if (interface->windowCloseInProgress_) {
//        logger->info("Window close already in progress, skipping this event.");
//        return;
//    }
//    interface->windowCloseInProgress_ = true;

    // focus the next plugin in the stack
    auto windows = AXFinder::getPluginWindowsFromLiveAX(1);
    if (windows.empty()) {
        logger->debug("no windows to focus");
        return;
    }
    AXUIElementRef windowToFocus = windows[0];
    AXError error = AXUIElementPerformAction(windowToFocus, kAXRaiseAction);
    if (error != kAXErrorSuccess) {
        logger->error("Failed to raise the next window. AXError: " + std::to_string(error));
    } else {
        CFRelease(windowToFocus);
        logger->debug("Successfully raised the next window.");
    }
//    interface->windowCloseInProgress_ = false;

    if (interface && interface->destroyCallback_) {
        interface->destroyCallback_();
    }
}

void LiveInterface::closeFocusedPlugin() {
    AXInteraction::closeFocusedPlugin();
}

void LiveInterface::closeAllPlugins() {
    AXInteraction::closeAllPlugins();
}

void LiveInterface::openAllPlugins() {
    AXInteraction::openAllPlugins();
}

// TODO -- if the plugins don't take up the full width or full height, center them
// TODO -- if the plugins take up MORE than the screen, cycle
// TODO -- compact tiling (fill in the blank space where possible while keeping order)

std::vector<AXUIElementRef> orderPluginWindows() {
    // find the devices in TrackView and toggle the ones that are enabled
    // to correctly order the plugins reported by Live AX
    std::vector<AXUIElementRef> trackViewDevices = AXFinder::getTrackViewDevices();
    for (const auto& device : trackViewDevices) {
        CFRetain(device);
        std::vector<AXUIElementRef> checkboxes = AXFinder::getTrackViewDeviceCheckBoxes(device);
        if (checkboxes.empty()) {
            logger->warn("couldn't find device on/off checkboxes");
            return {};
        }

        for (const auto& checkbox : checkboxes) {
            // TODO get the enable/disable checkbox and skip the ones that
            // aren't enabled
            AXCheckBox::toggleOffOn(checkbox);
            CFRelease(checkbox);
            usleep(20000);
        }

        CFRelease(device);
    }

    std::vector<AXUIElementRef> pluginWindows = AXFinder::getPluginWindowsFromLiveAX();
    if (pluginWindows.empty()) {
        logger->warn("no plugin windows found");
        return {};
    }

    std::reverse(pluginWindows.begin(), pluginWindows.end());
    
    return pluginWindows;
}

void LiveInterface::tilePluginWindows() {
    AXUIElementRef trackView = AXFinder::getTrackView();
    if (!AXAttribute::isValid(trackView)) {
        logger->warn("unable to find valid TrackView");
    }

    std::vector<AXUIElementRef> pluginWindows = orderPluginWindows();

    CGRect screenBounds = [[NSScreen mainScreen] frame];
    int screenWidth = screenBounds.size.width;
    logger->info("screen width: " + std::to_string(screenWidth));
    int screenHeight = screenBounds.size.height;

    int currentX = 0;
    int currentY = 0;
    int maxRowHeight = 0;
    int maxWidth = 0;
    int totalHeight = 0;

    std::vector<std::tuple<AXUIElementRef, int, int, int, int>> windowPositions;
    std::vector<AXUIElementRef> remainingWindows;

    int tallestWindow = 0;
    int rowStartX = 0;

    // First pass: calculate total width and height and store window positions
    for (const auto& window : pluginWindows) {
        CGRect bounds = AXWindow::getBounds(window);
        if (CGRectIsNull(bounds)) {
            logger->error("invalid window bounds");
            return;
        }

        int windowWidth = bounds.size.width;
        int windowHeight = bounds.size.height;

        if (currentX + windowWidth > screenWidth) {
            currentX = rowStartX;
            currentY += maxRowHeight;
            maxRowHeight = 0;
            tallestWindow = 0;
        }

        if (currentY + windowHeight > screenHeight) {
            remainingWindows.push_back(window);
            continue;
        }

        windowPositions.emplace_back(window, currentX, currentY, windowWidth, windowHeight);

        currentX += windowWidth;
        maxWidth = std::max(maxWidth, currentX);
        maxRowHeight = std::max(maxRowHeight, windowHeight);

        if (windowHeight > tallestWindow) {
            tallestWindow = windowHeight;
            rowStartX = windowWidth;
        }

        if (currentX == windowWidth) {  // First window in the row
            totalHeight += maxRowHeight;
        }
    }

    // Second pass: attempt to fit remaining windows in leftover space
    int leftoverWidth = screenWidth - maxWidth;
    int leftoverHeight = screenHeight - totalHeight;

    // Calculate the largest contiguous rectangle that remains unoccupied
    int leftX = 0;
    int topY = 0;
    int rightX = screenWidth;
    int bottomY = screenHeight;

    // Find the top-left corner of the unoccupied space
    for (const auto& [window, x, y, width, height] : windowPositions) {
        leftX = std::max(leftX, x + width);
        topY = std::max(topY, y + height);
    }

    // Update currentX and currentY to be the top-left corner of the unoccupied space
    currentX = leftX;
    currentY = topY;

    // Calculate the dimensions of the unoccupied rectangle
    int unoccupiedWidth = rightX - leftX;
    int unoccupiedHeight = bottomY - topY;

    logger->info("Unoccupied space: x=" + std::to_string(leftX) + 
                 ", y=" + std::to_string(topY) + 
                 ", width=" + std::to_string(unoccupiedWidth) + 
                 ", height=" + std::to_string(unoccupiedHeight));

    int unoccupiedMaxRowHeight = 0;
    int unoccupiedMaxRowWidth = 0;
    tallestWindow = 0;
    rowStartX = leftX;

    for (const auto& window : remainingWindows) {
        CGRect bounds = AXWindow::getBounds(window);
        int windowWidth = bounds.size.width;
        int windowHeight = bounds.size.height;

        if (currentX + windowWidth > rightX) {
            currentX = rowStartX;
            currentY += unoccupiedMaxRowHeight;
            unoccupiedMaxRowHeight = 0;
            tallestWindow = 0;
        }

        if (currentY + windowHeight > bottomY) {
            break;
        }

        windowPositions.emplace_back(window, currentX, currentY, windowWidth, windowHeight);

        currentX += windowWidth;
        unoccupiedMaxRowHeight = std::max(unoccupiedMaxRowHeight, windowHeight);
        unoccupiedMaxRowWidth = std::max(unoccupiedMaxRowWidth, currentX - leftX);

        if (windowHeight > tallestWindow) {
            tallestWindow = windowHeight;
            rowStartX = leftX + windowWidth;
        }
    }

    // Update maxWidth to include the unoccupied space
    maxWidth = std::max(maxWidth, leftX + unoccupiedMaxRowWidth);

    // Calculate offsets to center the formation
    int xOffset = (screenWidth - maxWidth) / 2;

    // Find the minimum y-coordinate among all windows
    int minY = std::numeric_limits<int>::max();
    for (const auto& [window, x, y, width, height] : windowPositions) {
        minY = std::min(minY, y);
    }

    // Calculate yOffset to maintain the original top position
    int yOffset = minY;

    // Third pass: actually position the windows
    for (auto& [window, x, y, width, height] : windowPositions) {
        x += xOffset;
        y += yOffset;

        AXPrinter::printAXIdentifier(window);
        logger->info("Setting bounds for window: x=" + std::to_string(x) + 
                     ", y=" + std::to_string(y) + 
                     ", width=" + std::to_string(width) + 
                     ", height=" + std::to_string(height));

        AXWindow::setBounds(window, x, y, width, height);
        usleep(20000);
    }

    // Release all window references
    for (const auto& [window, x, y, width, height] : windowPositions) {
        CFRelease(window);
    }
    for (const auto& window : remainingWindows) {
        CFRelease(window);
    }
}

//bool LiveInterface::isAnyTextFieldFocusedRecursive(AXUIElementRef parent, int level) {
////    logger->("recursion level " << level << " - Parent element: " << parent << std::endl;
//
//    if (level > 10) {
////        logger->("Max recursion depth reached at level " << level << std::endl;
//        return false;
//    }
//
//    if (!parent) {
//        logger->("Invalid parent element at level " << level << std::endl;
//        return false;
//    }
//
//    // Initialize children to nullptr
//    CFArrayRef children = nullptr;
//
//    // Retrieve the children of the parent element
//    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
//    if (error != kAXErrorSuccess || !children) {
////        logger->("Failed to retrieve children for the element. Error code: " << error << std::endl;
//        return false;  // Return false if no children found
//    }
//
//    CFIndex count = CFArrayGetCount(children);
////    logger->("Number of children at level " << level << ": " << count << std::endl;
//
//    for (CFIndex i = 0; i < count; i++) {
//        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
//        if (!child) {
////            logger->("No valid child found at index " << i << std::endl;
//            continue;  // Skip to next child if invalid
//        }
//
//        // Check if the element is a text field
//        CFTypeRef role = nullptr;
//        AXError roleError = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, &role);
//        if (roleError == kAXErrorSuccess && role) {
//            CFStringRef roleStr = static_cast<CFStringRef>(role);
//            if (CFStringCompare(roleStr, CFSTR("AXTextField"), 0) == kCFCompareEqualTo) {
//                CFRelease(role);
//
//                // Check if the text field is focused
//                CFTypeRef focusedValue;
//                AXError focusedError = AXUIElementCopyAttributeValue(child, kAXFocusedAttribute, &focusedValue);
//                if (focusedError == kAXErrorSuccess && focusedValue == kCFBooleanTrue) {
//                    logger->("Focused text field found at level " << level << std::endl;
//                    CFRelease(children);
//                    return true;  // Exit early if a focused text field is found
//                }
//                CFRelease(focusedValue);
//            } else {
//                CFRelease(role);
//            }
//        }
//
//        // Recursively search in child elements
//        if (isAnyTextFieldFocusedRecursive(child, level + 1)) {
//            CFRelease(children);  // Release children array before returning
//            return true;  // Exit early if a focused text field is found during recursion
//        }
//    }
//
//    // Release children if nothing is found
//    CFRelease(children);
//    return false;  // No focused text field found
//}

//bool LiveInterface::isAnyTextFieldFocused() {
//    AXElement window = AXElement(AXFinder::findApplicationWindow());
//    if (!window.isValid()) {
//        logger->("Window is null or invalid." << std::endl;
//        return false;
//    }
//
//    return isAnyTextFieldFocusedRecursive(window.getRef(), 0);
//}

// search box
//    if (window) {
//        AXUIElementRef liveSearchBox = findElementByIdentifier(window, cfIdentifier, 0);
//
//        CFRelease(window);
//        CFRelease(cfIdentifier);
//
//        if (liveSearchBox) {
//            logger->("Search box found." << std::endl;
//            printAllAttributes(liveSearchBox);
//            CFTypeRef focusedValue;
//            AXUIElementCopyAttributeValue(liveSearchBox, kAXFocusedAttribute, &focusedValue);
//            if (focusedValue == kCFBooleanTrue) {
//                std::cout << "Element is focused." << std::endl;
//            } else {
//                std::cout << "Element is not focused." << std::endl;
//            }
//            CFRelease(liveSearchBox);
//        }
//    } else {
//        logger->("Search box not found." << std::endl;
//    }

//void LiveInterface::searchFocusedTextField(AXUIElementRef parent) {
//    if (!parent) {
//        logger->("No parent element provided." << std::endl;
//        return;
//    }
//
//    // Retrieve the children of the parent element
//    CFArrayRef children = nullptr;
//    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
//
//    if (error != kAXErrorSuccess || !children) {
//        logger->("Unable to retrieve children for the element or no children found." << std::endl;
//        return;
//    }
//
//    CFIndex count = CFArrayGetCount(children);
//    for (CFIndex i = 0; i < count; i++) {
//        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
//        
//        // Check if the child has the AXRoleAttribute
//        CFTypeRef role;
//        AXError roleError = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, &role);
//        if (roleError == kAXErrorSuccess && role) {
//            CFStringRef roleStr = static_cast<CFStringRef>(role);
//
//            // Check if the child is a text field (AXTextFieldRole or TAxEditFieldElement)
//            if (CFStringCompare(roleStr, kAXTextFieldRole, 0) == kCFCompareEqualTo) {
//                // Check if this text field is focused
//                CFTypeRef isFocused = nullptr;
//                AXError focusedError = AXUIElementCopyAttributeValue(child, kAXFocusedAttribute, &isFocused);
//                
//                if (focusedError == kAXErrorSuccess && isFocused == kCFBooleanTrue) {
//                    std::cout << "Focused text field found" << std::endl;
//                    AXElement(child).print();  // Print info about the focused text field
//                    CFRelease(role);
//                    CFRelease(children);
//                    return;
//                }
//                CFRelease(isFocused);
//            }
//            CFRelease(role);
//        }
//
//        // Recursively search in child elements
//        searchFocusedTextField(child);
//    }
//
//    CFRelease(children);
//}

bool isPluginWindowTitle(const std::string& title) {
    // This regex matches titles like "aa/2-Audio"
    std::regex pattern(R"(^.*/.*)");
    return std::regex_match(title, pattern);
}

// Method to find and interact with the "Search, text field"
//void LiveInterface::findAndInteractWithSearchField() {
//    if (!mainWindow_) {
//        printf("Failed to get main window for app");
//        return;
//    }
//
//    CFArrayRef children = nullptr;
//    AXError error = AXUIElementCopyAttributeValue(mainWindow_, kAXChildrenAttribute, (CFTypeRef*)&children);
//    if (error != kAXErrorSuccess || !children) {
//        printf("Failed to get children of main window. Error: %d\n", error);
//        return;
//    }
//
//    std::unique_ptr<const __CFArray, decltype(&CFRelease)> childrenGuard(children, CFRelease);
//    
//    for (CFIndex i = 0; i < CFArrayGetCount(children); i++) {
//        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
//        if (!child) continue;
//
//        CFStringRef role = nullptr;
//        error = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, (CFTypeRef*)&role);
//        if (error != kAXErrorSuccess || !role) continue;
//
//        std::unique_ptr<const __CFString, decltype(&CFRelease)> roleGuard(role, CFRelease);
//        
//        if (CFStringCompare(role, kAXTextFieldRole, 0) == kCFCompareEqualTo) {
//            CFStringRef description = nullptr;
//            error = AXUIElementCopyAttributeValue(child, kAXDescriptionAttribute, (CFTypeRef*)&description);
//            if (error != kAXErrorSuccess || !description) continue;
//
//            std::unique_ptr<const __CFString, decltype(&CFRelease)> descriptionGuard(description, CFRelease);
//            
//            if (CFStringCompare(description, CFSTR("Search"), 0) == kCFCompareEqualTo) {
//                printf("Found the Search, text field in app\n");
//
//                if (!isElementFocused(child)) {
//                    printf("Search field is not focused. Focusing now...\n");
//                    AXElement(child).focus();
//                } else {
//                    printf("Search field is already focused.\n");
//                }
//
//                setTextInElement(child, "Hello, world!");
//                printf("Text sent to search field.\n");
//                return;
//            }
//        }
//    }
//
//    printf("Search field not found\n");
//}
