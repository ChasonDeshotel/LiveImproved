#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

#include <objc/runtime.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <regex>
#include <thread>
#include <vector>

#include "LogGlobal.h"

#include "AXElement.h"
#include "AXFinder.h"
#include "AXWindow.h"
#include "EventHandler.h"
#include "LiveInterface.h"
#include "PID.h"

// Constructor
LiveInterface::LiveInterface(std::function<std::shared_ptr<EventHandler>()> eventHandler)
    : ILiveInterface()
    , eventHandler_(std::move(eventHandler))
{
    setupPluginWindowChangeObserver([]() {
//        std::cout << "Window change detected!" << std::endl;
    });
}

// Destructor
LiveInterface::~LiveInterface() {}

void LiveInterface::setupPluginWindowChangeObserver(std::function<void()> callback) {
    pid_t livePID = PID::getInstance().livePID();
    if (livePID == -1) {
        logger->error("Live is not running");
        return;
    }

    logger->debug("set up plugin window change observer");

    AXElement appElement = AXFinder::appElement();
    if (!appElement.isValid()) {
        logger->error("unable to get app element");
        return;
    }
    
    // create the window create observer, set the callback
    AXError error = AXObserverCreate(livePID, pluginWindowCreateCallback, &pluginWindowCreateObserver_);
    if (error != kAXErrorSuccess) {
        std::cerr << "Failed to create accessibility observer. Error: " << error << std::endl;
        return;
    }

    // same for destroy
    error = AXObserverCreate(livePID, pluginWindowDestroyCallback, &pluginWindowDestroyObserver_);
    if (error != kAXErrorSuccess) {
        std::cerr << "Failed to create accessibility observer. Error: " << error << std::endl;
        return;
    }

    error = AXObserverAddNotification(pluginWindowCreateObserver_, appElement.getRef(), kAXCreatedNotification, this);
    if (error != kAXErrorSuccess) {
        std::cerr << "Failed to add creation notification. Error: " << error << std::endl;
        return;
    }

    // there is no window destroy observer, we'll have to check the type of what was destroyed
    error = AXObserverAddNotification(pluginWindowDestroyObserver_, appElement.getRef(), kAXUIElementDestroyedNotification, this);
    if (error != kAXErrorSuccess) {
        std::cerr << "Failed to add destruction notification. Error: " << error << std::endl;
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
    LiveInterface* interface = static_cast<LiveInterface*>(context);
    logger->info("create callback called");

    if (!AXElement(element).isPluginWindow()) return;

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

    LiveInterface* interface = static_cast<LiveInterface*>(context);
    logger->info("Destroy callback called");
//    if (interface->windowCloseInProgress_) {
//        logger->info("Window close already in progress, skipping this event.");
//        return;
//    }
//    interface->windowCloseInProgress_ = true;

    auto windows = AXFinder::getPluginWindowsFromLiveAX(1);
    if (windows.empty()) {
        logger->debug("no windows to focus");
        return;
    }
    AXElement windowToFocus = AXElement(windows[0]);
    AXError error = AXUIElementPerformAction(windowToFocus.getRef(), kAXRaiseAction);
    if (error != kAXErrorSuccess) {
        logger->error("Failed to raise the next window. AXError: " + std::to_string(error));
    } else {
        logger->info("Successfully raised the next window.");
//        interface->printAllAttributeValues(interface->getAppElement());
    }
//    interface->windowCloseInProgress_ = false;

    if (interface && interface->destroyCallback_) {
        interface->destroyCallback_();
    }
}

//bool LiveInterface::isAnyTextFieldFocusedRecursive(AXUIElementRef parent, int level) {
////    std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;
//
//    if (level > 10) {
////        std::cerr << "Max recursion depth reached at level " << level << std::endl;
//        return false;
//    }
//
//    if (!parent) {
//        std::cerr << "Invalid parent element at level " << level << std::endl;
//        return false;
//    }
//
//    // Initialize children to nullptr
//    CFArrayRef children = nullptr;
//
//    // Retrieve the children of the parent element
//    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
//    if (error != kAXErrorSuccess || !children) {
////        std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
//        return false;  // Return false if no children found
//    }
//
//    CFIndex count = CFArrayGetCount(children);
////    std::cerr << "Number of children at level " << level << ": " << count << std::endl;
//
//    for (CFIndex i = 0; i < count; i++) {
//        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
//        if (!child) {
////            std::cerr << "No valid child found at index " << i << std::endl;
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
//                    std::cerr << "Focused text field found at level " << level << std::endl;
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
//        std::cerr << "Window is null or invalid." << std::endl;
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
//            std::cerr << "Search box found." << std::endl;
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
//        std::cerr << "Search box not found." << std::endl;
//    }

//void LiveInterface::searchFocusedTextField(AXUIElementRef parent) {
//    if (!parent) {
//        std::cerr << "No parent element provided." << std::endl;
//        return;
//    }
//
//    // Retrieve the children of the parent element
//    CFArrayRef children = nullptr;
//    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
//
//    if (error != kAXErrorSuccess || !children) {
//        std::cerr << "Unable to retrieve children for the element or no children found." << std::endl;
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
