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
#include "EventHandler.h"
#include "LiveInterface.h"
#include "PID.h"

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef element, CGWindowID* windowID);

// Constructor
LiveInterface::LiveInterface(std::function<std::shared_ptr<EventHandler>()> eventHandler)
    : ILiveInterface()
    , eventHandler_(std::move(eventHandler))
    , pluginWindows_()
{
    setupPluginWindowChangeObserver([]() {
//        std::cout << "Window change detected!" << std::endl;
    });
}

// Destructor
LiveInterface::~LiveInterface() {}

void LiveInterface::printElementInfo(AXUIElementRef element, std::string prefix = "") {
    CFStringRef roleStr, subrole, title, identifier;
    char buffer[256];

    if (!isElementValid(element)) return;
    
    CGWindowID windowID;
    AXError error = _AXUIElementGetWindow(element, &windowID);
    if (error == kAXErrorSuccess) {
        prefix += "ID: " + std::to_string(static_cast<int>(windowID));
//        std::cout << prefix << "ID: " << windowID << std::endl;
    }
//    if (AXUIElementCopyAttributeValue(element, kAXRoleAttribute, (CFTypeRef*)&role) == kAXErrorSuccess) {
//        CFStringGetCString(role, buffer, sizeof(buffer), kCFStringEncodingUTF8);
//        std::cout << prefix << "Role: " << buffer << std::endl;
//        CFRelease(role);
//    }
//
//    if (AXUIElementCopyAttributeValue(element, kAXSubroleAttribute, (CFTypeRef*)&subrole) == kAXErrorSuccess) {
//        CFStringGetCString(subrole, buffer, sizeof(buffer), kCFStringEncodingUTF8);
//        std::cout << prefix << "Subrole: " << buffer << std::endl;
//        CFRelease(subrole);
//    }

    if (AXUIElementCopyAttributeValue(element, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess) {
        CFStringGetCString(title, buffer, sizeof(buffer), kCFStringEncodingUTF8);
        std::cout << prefix << "  Title: " << buffer << std::endl;
        CFRelease(title);
    }

//    if (AXUIElementCopyAttributeValue(element, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess) {
//        CFStringGetCString(identifier, buffer, sizeof(buffer), kCFStringEncodingUTF8);
//        std::cout << prefix << "Identifier (Class): " << buffer << std::endl;
//        CFRelease(identifier);
//    }
    CFTypeRef attributeValue = nullptr;
    const char* cString = "AXFocused";
    CFStringRef attributeName = CFStringCreateWithCString(NULL, cString, kCFStringEncodingUTF8);

    error = AXUIElementCopyAttributeValue(element, attributeName, &attributeValue);
    CFRelease(attributeName);
    if (error == kAXErrorSuccess && attributeValue != nullptr) {
        if (CFGetTypeID(attributeValue) == CFBooleanGetTypeID()) {
            // If the value is a boolean
            Boolean boolValue = CFBooleanGetValue((CFBooleanRef)attributeValue);
            std::cout << prefix << "AXFocused: " << (boolValue ? "true" : "false") << std::endl;
        }
    }
}

std::string getStringFromCFString(CFStringRef cfString) {
    if (!cfString) return "";
    
    CFIndex length = CFStringGetLength(cfString);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::vector<char> buffer(maxSize);
    
    if (CFStringGetCString(cfString, buffer.data(), maxSize, kCFStringEncodingUTF8)) {
        return std::string(buffer.data());
    }
    return "";
}

std::vector<AXUIElementRef> LiveInterface::getPluginWindowsFromLiveAX(int limit = -1) {
    logger->debug("getPluginWindowsFromLiveAX called");

    CFArrayRef children;
    AXUIElementRef appElement = getAppElement();
    AXError error = AXUIElementCopyAttributeValues(appElement, kAXChildrenAttribute, 0, 100, &children);
    if (error != kAXErrorSuccess) {
        logger->error(&"Failed to get children of app. Error: " [error]);
        CFRelease(appElement);
        return {};
    }

    std::vector<AXUIElementRef> pluginWindowsFromLiveAX;
    CFIndex childCount = CFArrayGetCount(children);
    logger->debug("child count: " + std::to_string(static_cast<int>(childCount)));

    for (CFIndex i = 0; i < childCount; i++) {
        if (limit != -1 && pluginWindowsFromLiveAX.size() >= static_cast<size_t>(limit)) {
            logger->debug("Limit reached, stopping collection of plugin windows.");
            break;
        }
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        if (isPluginWindow(child)) {
            logger->debug("getCurrentPluginsWindow - Child " + std::to_string(static_cast<int>(i)));
            printElementInfo(child, "  plugin window: ");
            CFRetain(child);
            pluginWindowsFromLiveAX.push_back(child);
        } else {
            printElementInfo(child, "  other child: ");
        }
    }

    if (appElement) CFRelease (appElement);
    if (children) CFRelease (children);
    
    return pluginWindowsFromLiveAX;
}

bool LiveInterface::isAnyPluginWindowFocused() {
    AXUIElementRef focusedPlugin = getFocusedPluginWindow();
    return focusedPlugin != nullptr;
}

AXUIElementRef LiveInterface::getFocusedPluginWindow() {
    std::vector<AXUIElementRef> pluginWindows = pluginWindows_;
    
    std::cout << "Number of plugin windows: " << pluginWindows.size() << std::endl;

    for (const auto& window : pluginWindows) {
        CFBooleanRef focusedValue;
        AXError error = AXUIElementCopyAttributeValue(window, kAXFocusedAttribute, (CFTypeRef*)&focusedValue);
        
        if (error == kAXErrorSuccess) {
            bool isFocused = CFBooleanGetValue(focusedValue);
            CFRelease(focusedValue);
            
            std::cout << "Window focus state: " << (isFocused ? "focused" : "not focused") << std::endl;

            if (isFocused) {
                return window;
            }
        } else {
            std::cerr << "Failed to get focus state. Error: " << error << std::endl;
        }
    }
    
    std::cout << "No focused window found" << std::endl;
    return nullptr;
}

AXUIElementRef LiveInterface::getAppElement() {
    pid_t livePID = PID::getInstance().livePID();
    if (livePID == -1) {
        logger->error("Live is not running");
        return nullptr;
    }

    AXUIElementRef appElement = AXUIElementCreateApplication(livePID);
    if (appElement == nullptr) {
        std::cerr << "Failed to create application element for Live" << std::endl;
        return nullptr;
    }
    std::cout << "Successfully created application element" << std::endl;
    return appElement;
}

void LiveInterface::setupPluginWindowChangeObserver(std::function<void()> callback) {
    pid_t livePID = PID::getInstance().livePID();
    if (livePID == -1) {
        logger->error("Live is not running");
        return;
    }

    logger->debug("set up plugin window change observer");

    AXUIElementRef appElement = getAppElement();
    if (!appElement) {
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

    error = AXObserverAddNotification(pluginWindowCreateObserver_, appElement, kAXCreatedNotification, this);
    if (error != kAXErrorSuccess) {
        std::cerr << "Failed to add creation notification. Error: " << error << std::endl;
        return;
    }

    // there is no window destroy observer, we'll have to check the type of what was destroyed
    error = AXObserverAddNotification(pluginWindowDestroyObserver_, appElement, kAXUIElementDestroyedNotification, this);
    if (error != kAXErrorSuccess) {
        std::cerr << "Failed to add destruction notification. Error: " << error << std::endl;
        return;
    }
    CFRelease(appElement);

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

bool LiveInterface::isPluginWindow(AXUIElementRef element) {
    if (!element) {
        logger->debug("element is null");
        return false;
    }
    
    CFStringRef role;
    AXUIElementCopyAttributeValue(element, kAXRoleAttribute, (CFTypeRef*)&role);
    if (!role) {
        logger->debug("error getting role");
        return false;
    }
    
    // not a window
    if (! (CFStringCompare(role, kAXWindowRole, 0) == kCFCompareEqualTo)) {
        if (role) CFRelease(role);
        logger->debug("not an AXWindow");
        return false;
    }

    // not a floating window
    CFStringRef subrole;
    AXUIElementCopyAttributeValue(element, kAXSubroleAttribute, (CFTypeRef*)&subrole);
    if (CFStringCompare(subrole, kAXFloatingWindowSubrole, 0) == kCFCompareEqualTo) {
        if (role) CFRelease(role);
        if (subrole) CFRelease(subrole);
        return true;
    }
    logger->debug("not an AXFloatingWindow");

    return false;
}

bool LiveInterface::isElementValid(AXUIElementRef element) {
    CFTypeRef role = nullptr;
    AXError result = AXUIElementCopyAttributeValue(element, kAXRoleAttribute, &role);
        
    if (result != kAXErrorSuccess || role == nullptr) {
        logger->warn("AXUIElementRef is invalid, failed to get role attribute!");
        return false;
    }

    CFRelease(role);
    return true;
}

void LiveInterface::pluginWindowCreateCallback(AXObserverRef observer, AXUIElementRef element, 
                                         CFStringRef notification, void* context) {
    LiveInterface* interface = static_cast<LiveInterface*>(context);
    logger->info("create callback called");

    if (!interface->isPluginWindow(element)) return;

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

    auto windows = interface->getPluginWindowsFromLiveAX(1);
    if (windows.empty()) {
        logger->debug("no windows to focus");
        return;
    }
    AXUIElementRef windowToFocus = windows[0];
    AXError error = AXUIElementPerformAction(windowToFocus, kAXRaiseAction);
    if (error != kAXErrorSuccess) {
        logger->error("Failed to raise the next window. AXError: " + std::to_string(error));
    } else {
        logger->info("Successfully raised the next window.");
//        interface->printAllAttributeValues(interface->getAppElement());
    }
    CFRelease(windowToFocus);

//    interface->windowCloseInProgress_ = false;

    if (interface && interface->destroyCallback_) {
        interface->destroyCallback_();
    }
}

AXUIElementRef LiveInterface::getFocusedElement() {
    AXUIElementRef appElement = getAppElement();
    AXUIElementRef focusedElement = nullptr;

    AXError error = AXUIElementCopyAttributeValue(appElement, kAXFocusedUIElementAttribute, (CFTypeRef*)&focusedElement);
    if (error == kAXErrorSuccess && focusedElement != nullptr) {
        printAllAttributeValues(focusedElement);
        CFRelease(focusedElement);
        return(focusedElement);
    }

//    if (error == kAXErrorSuccess && focusedElement != nullptr) {
//        for (AXUIElementRef pluginWindow : pluginWindows) {
//            if (CFEqual(focusedElement, pluginWindow)) {
//                logger->info("One of the plugin windows is currently focused.");
//                // You can take actions based on the focus state
//            }
//        }
//        CFRelease(focusedElement);
//    } else {
//        logger->warn("No focused element found.");
//    }

    CFRelease(appElement);
    return nullptr;
}


void LiveInterface::printAXElementChildrenRecursively(AXUIElementRef element, int depth = 5, int currentDepth = 0) {
    if (currentDepth >= depth) {
        return; // Stop recursion when reaching the depth limit
    }

    CFTypeRef childrenValue = nullptr;
    AXError error = AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, &childrenValue);

    if (error == kAXErrorSuccess && childrenValue != nullptr) {
        if (CFGetTypeID(childrenValue) == CFArrayGetTypeID()) {
            CFArrayRef children = (CFArrayRef)childrenValue;
            CFIndex childCount = CFArrayGetCount(children);

            for (CFIndex i = 0; i < childCount; i++) {
                AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);

                // Call printAllAttributeValues on each child
                printAllAttributeValues(child);

                // Recursively go through the child's children
                printAXElementChildrenRecursively(child, depth, currentDepth + 1);
            }
        }
        CFRelease(childrenValue); // Don't forget to release when done
    }
}

AXUIElementRef LiveInterface::getFrontmostWindow() {
    AXUIElementRef frontmostWindow;
    AXUIElementRef appElement = getAppElement();

    AXError result = AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute, (CFTypeRef *)&frontmostWindow);
    
    if (result == kAXErrorSuccess && frontmostWindow) {
        CFRelease(appElement);
        logger->debug("Successfully obtained the frontmost window.");
    } else {
        logger->debug("Failed to get the frontmost window.");
    }

    return frontmostWindow;
}

CFArrayRef LiveInterface::getAllWindows() {
    CFArrayRef windows;
    AXError result = AXUIElementCopyAttributeValue(getAppElement(), kAXWindowsAttribute, (CFTypeRef *)&windows);
    
    if (result == kAXErrorSuccess && windows) {
        CFIndex windowCount = CFArrayGetCount(windows);
        logger->debug("Number of windows: " + std::to_string(windowCount));
        
        for (CFIndex i = 0; i < windowCount; i++) {
            AXUIElementRef window = (AXUIElementRef)CFArrayGetValueAtIndex(windows, i);
            
            CFStringRef windowTitle;
            AXUIElementCopyAttributeValue(window, kAXTitleAttribute, (CFTypeRef *)&windowTitle);
            
            if (windowTitle) {
                // Convert CFStringRef to std::string
                NSString *nsString = (__bridge NSString *)windowTitle;
                std::string title([nsString UTF8String]);
                logger->debug("Window " + std::to_string(i) + ": " + title);
                CFRelease(windowTitle);
            } else {
                logger->debug("Window " + std::to_string(i) + ": (No Title)");
            }
        }
        return windows;
        CFRelease(windows);
    } else {
        logger->debug("Failed to get windows.");
    }
    return nullptr;
}

#include <algorithm> // For std::reverse

// Helper function to print CFString safely
void printCFString(CFStringRef str) {
    if (str == nullptr) return;

    const int bufferSize = 256;
    char buffer[bufferSize];
    if (CFStringGetCString(str, buffer, bufferSize, kCFStringEncodingUTF8)) {
        std::cout << buffer << std::endl;
    } else {
        std::cerr << "Unable to retrieve CFString value." << std::endl;
    }
}
void printAXTree(AXUIElementRef element, int level) {
    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, (CFTypeRef*)&children);

    if (error == kAXErrorSuccess && children) {
        CFIndex count = CFArrayGetCount(children);
        for (CFIndex i = 0; i < count; ++i) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);

            // Get and print the title of each child (if available)
            CFStringRef title = nullptr;
            if (AXUIElementCopyAttributeValue(child, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess && title) {
                std::cout << std::string(level * 2, ' ') << "Title: ";
                printCFString(title);
                CFRelease(title);
            } else {
                std::cout << std::string(level * 2, ' ') << "Title: (none)" << std::endl;
            }

            // Get and print the identifier of each child (if available)
            CFStringRef identifier = nullptr;
            if (AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
                std::cout << std::string(level * 2, ' ') << "Identifier: ";
                printCFString(identifier);
                CFRelease(identifier);
            } else {
                std::cout << std::string(level * 2, ' ') << "Identifier: (none)" << std::endl;
            }

            // Recursively print the tree
            printAXTree(child, level + 1);
        }
        CFRelease(children);
    } else {
        std::cerr << std::string(level * 2, ' ') << "Failed to retrieve children or no children." << std::endl;
    }
}

AXUIElementRef findAXMain(AXUIElementRef parent) {
    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);

    if (error != kAXErrorSuccess || !children) {
        std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
        return nullptr;
    }

    // Ensure children is released at the end of the function
    std::unique_ptr<const __CFArray, decltype(&CFRelease)> childrenGuard(children, CFRelease);

    CFIndex count = CFArrayGetCount(children);
    for (CFIndex i = 0; i < count; ++i) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);

        // Check the AXMain attribute
        CFTypeRef attributeValue = nullptr;
        AXError attrError = AXUIElementCopyAttributeValue(child, kAXMainAttribute, &attributeValue);

        if (attrError == kAXErrorSuccess && attributeValue == kCFBooleanTrue) {
            CFRelease(attributeValue);
            std::cerr << "Match found: AXMain = true" << std::endl;
            CFRetain(child);
            return child;
        }

        if (attributeValue) {
            CFRelease(attributeValue);
        }

        // Recursively search in child elements
        AXUIElementRef found = findAXMain(child);
        if (found) {
            CFRetain(found);
            return found;
        }
    }

    return nullptr;
}


bool LiveInterface::isAXCheckBoxChecked(AXUIElementRef elem) {
    CFBooleanRef value = nullptr;
    if (AXUIElementCopyAttributeValue(elem, kAXValueAttribute, (CFTypeRef*)&value) == kAXErrorSuccess && value) {
        bool isChecked = CFBooleanGetValue(value);  // Convert CFBooleanRef to bool
        CFRelease(value);  // Release CFBooleanRef after use
        return isChecked;
    } else {
        std::cerr << "AXValue attribute not found or failed to retrieve for checkbox." << std::endl;
        return false;  // Return false if the value is not found or retrieval failed
    }
}
// .PluginEdit - for toggle

bool LiveInterface::pressAXCheckBox(AXUIElementRef checkbox) {
    if (!checkbox) {
        std::cerr << "Error: AXUIElementRef is null." << std::endl;
        return false;
    }

    // Perform the AXPress action on the checkbox
    AXError error = AXUIElementPerformAction(checkbox, kAXPressAction);

    if (error == kAXErrorSuccess) {
        logger->info("Successfully pressed the checkbox.");
        return true;
    } else {
        std::cerr << "Failed to press the checkbox. Error: " << error << std::endl;
        return false;
    }
}

bool LiveInterface::toggleOffOnAXCheckBox(AXUIElementRef checkbox) {
    if (!isAXCheckBoxChecked(checkbox)) {
        logger->warn("not checked - already on");
        return false;
    }
    if (isAXCheckBoxChecked(checkbox)) {
        bool offPress = pressAXCheckBox(checkbox);
        usleep(20000);
        bool onPress = pressAXCheckBox(checkbox);
        if (onPress && offPress) {
            return true;
        }
    }
    return false;
}

// Device On is the on/off switch
std::vector<AXUIElementRef> LiveInterface::getTrackViewDeviceCheckBoxes(AXUIElementRef deviceElement) {
    if (!deviceElement) {
        std::cerr << "Error: deviceElement is null." << std::endl;
        return {};
    }
    
    std::vector<AXUIElementRef> checkBoxElements;

    printAXTitle(deviceElement);

    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(deviceElement, kAXChildrenAttribute, (CFTypeRef*)&children);

    if (error == kAXErrorSuccess && children) {
        CFIndex count = CFArrayGetCount(children);

        // Iterate over the children of TrackView.Device[i]
        for (CFIndex i = 0; i < count; i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);

            // Check if the child has the identifier TrackView.Device[i].TitleBar
            CFStringRef identifier = nullptr;
            if (AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
                char identifierBuffer[256];
                if (CFStringGetCString(identifier, identifierBuffer, sizeof(identifierBuffer), kCFStringEncodingUTF8)) {
                    std::string identifierStr(identifierBuffer);

                    // Look for the TitleBar element
                    if (identifierStr.find("PluginEdit") != std::string::npos) {
                        std::cout << "Found PluginEdit: " << identifierStr << std::endl;

//                        bool isChecked = isAXCheckBoxChecked(child);
//                        logger->info("checkbox state: " + std::string(isChecked ? "true" : "false"));
                        CFRetain(child);
                        checkBoxElements.push_back(child);

                        CFRelease(identifier);
                    }
                }
            }
        }
        CFRelease(children);
        if (checkBoxElements.size()) return checkBoxElements;
    }

    std::cerr << "Failed to find the DeviceOn element." << std::endl;
    return {};
}


AXUIElementRef LiveInterface::getTrackView() {
    AXUIElementRef appElement = getAppElement();
    AXUIElementRef axMain = findAXMain(appElement);
    if (!axMain) {
        return nullptr;
    }
    //printAXTree(axMain, 0);
    //printAllAttributes(axMain);

    CFStringRef valueToFind = CFSTR("TrackView");
    CFStringRef searchAttribute = kAXIdentifierAttribute;
    AXUIElementRef foundElement = findElementByAttribute(axMain, valueToFind, searchAttribute, 0, 1);
    CFRelease(axMain);

    if (foundElement) {
        logger->info("found track view");
        //printAllAttributeValues(foundElement);
        CFRetain(foundElement);
        return foundElement;
    }
    return axMain;
}

void LiveInterface::printAXTitle(AXUIElementRef elem) {
    return; // TODO
    CFStringRef title = nullptr;
    if (AXUIElementCopyAttributeValue(elem, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess && title) {
        std::cout << " Title: ";
        printCFString(title);
        CFRelease(title);
    } else {
        std::cout << " Title: (none)" << std::endl;
    }
}

bool LiveInterface::getAXEnabled(AXUIElementRef elem) {
    CFBooleanRef enabled = nullptr;
    if (AXUIElementCopyAttributeValue(elem, kAXEnabledAttribute, (CFTypeRef*)&enabled) == kAXErrorSuccess && enabled) {
        bool isEnabled = CFBooleanGetValue(enabled);  // Convert CFBooleanRef to bool
        CFRelease(enabled);  // Release the CFBooleanRef after use
        return isEnabled;
    } else {
        std::cerr << "AXEnabled attribute not found or failed to retrieve." << std::endl;
        return false;  // Return false if the element doesn't have the AXEnabled attribute or retrieval failed
    }
}

void printAXIdentifier(AXUIElementRef elem) {
    CFStringRef identifier = nullptr;
    if (AXUIElementCopyAttributeValue(elem, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
        std::cout << " Identifier: ";
        printCFString(identifier);
        CFRelease(identifier);
    } else {
        std::cout << " Identifier: (none)" << std::endl;
    }
}

#ifndef kAXChildrenInNavigationOrderAttribute
#define kAXChildrenInNavigationOrderAttribute CFSTR("AXChildrenInNavigationOrder")
#endif

void LiveInterface::printAXChildrenInNavigationOrder(AXUIElementRef element) {
    if (!element) {
        std::cerr << "Error: AXUIElementRef is null." << std::endl;
        return;
    }

    CFArrayRef navigationChildren = nullptr;
    AXError error = AXUIElementCopyAttributeValue(element, kAXChildrenInNavigationOrderAttribute, (CFTypeRef*)&navigationChildren);

    if (error == kAXErrorSuccess && navigationChildren) {
        CFIndex count = CFArrayGetCount(navigationChildren);
        std::cout << "Number of children in navigation order: " << count << std::endl;

        for (CFIndex i = 0; i < count; i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(navigationChildren, i);

            // Retrieve and print the identifier of each child
            CFStringRef identifier = nullptr;
            if (AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
                std::cout << " Child " << i << " Identifier: ";
                printCFString(identifier);  // Helper function to print CFStringRef
                CFRelease(identifier);
            } else {
                std::cout << " Child " << i << " Identifier: (none)" << std::endl;
            }

            // Optionally, you could print other attributes, like title or role
        }

        CFRelease(navigationChildren);  // Release the array after use
    } else {
        std::cerr << "Failed to retrieve AXChildrenInNavigationOrder or no children found." << std::endl;
    }
}


//  Identifier: TrackView.Device[0].TitleBar.DeviceOn
//  Identifier: TrackView.Device[0].TitleBar.UnfoldDeviceParameters
//  Identifier: TrackView.Device[0].TitleBar.PluginEdit
//  Identifier: TrackView.Device[0].TitleBar.device_title
//  Identifier: TrackView.Device[0].TitleBar.ConfigureMode

// Found PluginEdit: TrackView.Device[2].TitleBar.PluginEdit
// Attribute: AXIdentifier
// Attribute: AXEnabled
// Attribute: AXFrame
// Attribute: AXDescription
// Attribute: AXParent
// Attribute: AXChildren
// Attribute: AXFocused
// Attribute: AXActivationPoint
// Attribute: AXRole
// Attribute: AXTopLevelUIElement
// Attribute: AXHelp
// Attribute: AXValue
// Attribute: AXChildrenInNavigationOrder
// Attribute: AXWindow
// Attribute: AXRoleDescription
// Attribute: AXTitle
// Attribute: AXSubrole
// Attribute: AXSize
// Attribute: AXPosition

std::vector<AXUIElementRef> LiveInterface::getTrackViewDevices() {
    std::vector<AXUIElementRef> trackViewDevices;

    AXUIElementRef trackView = getTrackView();
//    printAXTree(trackView, 0);

    if (trackView) {

        CFArrayRef children = nullptr;
        AXError error = AXUIElementCopyAttributeValue(trackView, kAXChildrenAttribute, (CFTypeRef*)&children);
        CFRelease(trackView);  // Release trackView after use

        if (error == kAXErrorSuccess && children) {
            CFIndex count = CFArrayGetCount(children);
            for (CFIndex i = 0; i < count; ++i) {
                AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
                CFRetain(child);
                trackViewDevices.push_back(child);

                CFStringRef identifier = nullptr;
                if (AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, (CFTypeRef*)&identifier) == kAXErrorSuccess && identifier) {
                    std::cout << " Identifier: ";
                    printCFString(identifier);
                    CFRelease(identifier);
                } else {
                    std::cout << " Identifier: (none)" << std::endl;
                }
            }
            CFRelease(children);
            return trackViewDevices;
        } else {
            std::cerr << "Failed to retrieve children or no children." << std::endl;
        }
    }

    return {};
}

void LiveInterface::tilePluginWindows() {
    AXUIElementRef trackView = getTrackView();
    std::vector<AXUIElementRef> trackViewDevices = getTrackViewDevices();

    for (const auto& device : trackViewDevices) {
//        printAXIdentifier(device);
//        printAXTree(device, 0);
        CFRetain(device);
        std::vector<AXUIElementRef> checkboxes = getTrackViewDeviceCheckBoxes(device);

        for (const auto& checkbox : checkboxes) {
            toggleOffOnAXCheckBox(checkbox);
            if (checkbox) CFRelease(checkbox);
            usleep(20000);
        }

        CFRelease(device);
    }

    return;

//    if (trackView) {
//        printAXTree(trackView, 0);
//        CFRelease(trackView);
//    }
//
//    return;


//    printAXTree(getAppElement(), 0);
//    return;
//    CFStringRef valueToFind = CFSTR("Trackview");
//    AXUIElementRef deviceDetailGroup = findElementByIdentifier(getAppElement(), valueToFind, 0);
//    if (deviceDetailGroup) {
//        logger->info("found device detail group");
//        printAXTree(deviceDetailGroup, 0);
//    }
//    return;

    std::vector<AXUIElementRef> pluginWindows = getPluginWindowsFromLiveAX();
    if (pluginWindows.empty()) return;

    std::reverse(pluginWindows.begin(), pluginWindows.end());

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
    //double scaleFactor = std::sqrt((screenWidth * screenHeight) / totalArea);
    double scaleFactor = 1;

    // Calculate the number of rows and columns
    int numWindows = static_cast<int>(pluginWindows.size());
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

        // If adding this window would go past the right edge of the screen, move to the next row
        if (currentX + scaledWidth > screenWidth) {
            currentX = 0;  // Reset X to the left
            currentY += maxRowHeight;  // Move down to the next row
            maxRowHeight = 0;  // Reset row height for the new row
        }

        // If the next row would go off the screen, break the loop (optional if you want to stop tiling)
        if (currentY + scaledHeight > screenHeight) {
            break;
        }

        // Set the window bounds with the current calculated position
        setWindowBounds(pluginWindows[i], currentX, currentY, scaledWidth, scaledHeight);

        // Move the X position for the next window
        currentX += scaledWidth;

        // Keep track of the tallest window in the row
        maxRowHeight = std::max(maxRowHeight, scaledHeight);
    }
}

void LiveInterface::setWindowBounds(AXUIElementRef window, int x, int y, int width, int height) {
    CGPoint position = {static_cast<CGFloat>(x), static_cast<CGFloat>(y)};
    CGSize size = {static_cast<CGFloat>(width), static_cast<CGFloat>(height)};

    AXValueRef positionRef = AXValueCreate(static_cast<AXValueType>(kAXValueCGPointType), &position);
    AXUIElementSetAttributeValue(window, kAXPositionAttribute, positionRef);
    CFRelease(positionRef);

    AXValueRef sizeRef = AXValueCreate(static_cast<AXValueType>(kAXValueCGSizeType), &size);
    AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeRef);
    CFRelease(sizeRef);

    // Update the cached bounds
    cachedWindowBounds_[window] = CGRectMake(x, y, width, height);
}

CGRect LiveInterface::getWindowBounds(AXUIElementRef window) {
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

    AXValueGetValue(positionRef, static_cast<AXValueType>(kAXValueCGPointType), &position);
    AXValueGetValue(sizeRef, static_cast<AXValueType>(kAXValueCGSizeType), &size);

    CGRect bounds = CGRectMake(position.x, position.y, size.width, size.height);

    // Cache the bounds
    cachedWindowBounds_[window] = bounds;

    if (positionRef) CFRelease(positionRef);
    if (sizeRef) CFRelease(sizeRef);

    return bounds;
}

void LiveInterface::closeFocusedPluginWindow() {
    AXUIElementRef frontmostWindow = getFrontmostWindow();

    CFStringRef frontmostTitle = nullptr;
    AXError error = AXUIElementCopyAttributeValue(frontmostWindow, kAXTitleAttribute, (CFTypeRef *)&frontmostTitle);
    
    if (error != kAXErrorSuccess || !frontmostTitle) {
        logger->debug("Failed to get title of frontmost window.");
        CFRelease(frontmostWindow);
        return;
    }

    // Convert the frontmost window title to a std::string
    NSString *frontmostTitleNS = (__bridge NSString *)frontmostTitle;
    std::string frontmostTitleStr([frontmostTitleNS UTF8String]);

    // Get plugin windows from Live
    auto windows = getPluginWindowsFromLiveAX();
    if (windows.empty()) {
        logger->debug("No plugin windows found.");
        CFRelease(frontmostTitle);
        CFRelease(frontmostWindow);
        return;
    }

    // if the frontmost window != a plugin window title, we bail
    bool found = false;
    for (const auto& pluginWindow : windows) {
        // Get the title of the plugin window
        CFStringRef pluginTitle = nullptr;
        AXError pluginError = AXUIElementCopyAttributeValue(pluginWindow, kAXTitleAttribute, (CFTypeRef *)&pluginTitle);

        if (pluginError == kAXErrorSuccess && pluginTitle) {
            // Convert the plugin window title to a std::string
            NSString *pluginTitleNS = (__bridge NSString *)pluginTitle;
            std::string pluginTitleStr([pluginTitleNS UTF8String]);

            // Compare titles
            if (frontmostTitleStr == pluginTitleStr) {
                logger->debug("Frontmost window matches plugin window by title.");
                found = true;
                CFRelease(pluginTitle); // Release after use
                break;
            }
            CFRelease(pluginTitle); // Release after use
        }
    }
    if (!found) {
        logger->debug("not found - returning");
        return;
    }

//    printAXElementChildrenRecursively(getAppElement());

//    //printAllAttributeValues(getAppElement());

    AXUIElementRef highestPlugin = windows[0];

    CGWindowID windowID;
    error = _AXUIElementGetWindow(highestPlugin, &windowID);
    if (error == kAXErrorSuccess) {
        logger->debug("ID: " + std::to_string(static_cast<int>(windowID)));
//        if (eventHandler_()->isWindowFocused(static_cast<int>(windowID))) {
//            logger->debug("closing window");
            closeSpecificWindow(highestPlugin);
//        }
    }

    CFRelease(highestPlugin);
}

void LiveInterface::closeSpecificWindow(WindowHandle windowHandle) {
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

// Method to get the application's main window by its PID
AXUIElementRef LiveInterface::findApplicationWindow() {
    AXUIElementRef appElement = AXUIElementCreateApplication(PID::getInstance().livePID());
    
    AXUIElementRef window = nullptr;
    if (appElement) {
        // Get the main window of the application
        AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute, (CFTypeRef*)&window);
        CFRelease(appElement);
        //std::cerr << "main window found - PID " + std::to_string(PID::getInstance().livePID()) << std::endl;
    } else {
        std::cerr << "no window found - PID " + std::to_string(PID::getInstance().livePID()) << std::endl;
    }
    mainWindow_ = window;
    return window;
}

//AXUIElementRef LiveInterface::findSearchBox() {
//    mainWindow_
//}
//
//AXUIElementRef LiveInterface::getFocusedElement() {
//    mainWindow_.kAXFocusedUIElementAttribute
//}
void LiveInterface::printFocusedElementInChildren(AXUIElementRef parent) {
    if (!parent) {
        std::cerr << "No parent element provided." << std::endl;
        return;
    }

    // Retrieve the children of the parent element
    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);

    if (error != kAXErrorSuccess || !children) {
        std::cerr << "Unable to retrieve children for the element or no children." << std::endl;
        return;
    }

    CFIndex count = CFArrayGetCount(children);
    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        
        // Check if this child is focused
        AXUIElementRef focusedElement = nullptr;
        AXError focusedError = AXUIElementCopyAttributeValue(child, kAXFocusedUIElementAttribute, (CFTypeRef*)&focusedElement);

        if (focusedError == kAXErrorSuccess && focusedElement) {
            std::cout << "Focused element found!" << std::endl;
            printElementInfo(focusedElement);  // Print the info for the focused element
            CFRelease(focusedElement);
            CFRelease(children);  // Release children array when we find the focused element
            return;
        } else {
            // Recursively search in child elements
            printFocusedElementInChildren(child);
        }
    }

    CFRelease(children);  // Release children array if no focused element is found
}

void printChildren(AXUIElementRef element, int level = 0) {
    if (!element) {
        std::cerr << "Invalid AXUIElementRef." << std::endl;
        return;
    }

    // Indentation for the hierarchy level
    std::string indent(level * 2, ' ');

    // Get the role of the element
    CFTypeRef role = nullptr;
    AXError roleError = AXUIElementCopyAttributeValue(element, kAXRoleAttribute, &role);
    if (roleError == kAXErrorSuccess && role) {
        CFStringRef roleStr = static_cast<CFStringRef>(role);
        char buffer[256];
        if (CFStringGetCString(roleStr, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
            std::cout << indent << "Role: " << buffer << std::endl;
        }
        CFRelease(role);
    } else {
        std::cerr << indent << "Failed to get role for element. Error: " << roleError << std::endl;
        return;  // If we can't get the role, there's no point continuing
    }

    // Optionally print other attributes (like title)
    CFTypeRef title = nullptr;
    AXError titleError = AXUIElementCopyAttributeValue(element, kAXTitleAttribute, &title);
    if (titleError == kAXErrorSuccess && title) {
        CFStringRef titleStr = static_cast<CFStringRef>(title);
        char titleBuffer[256];
        if (CFStringGetCString(titleStr, titleBuffer, sizeof(titleBuffer), kCFStringEncodingUTF8)) {
            std::cout << indent << "Title: " << titleBuffer << std::endl;
        }
        CFRelease(title);
    } else if (titleError != kAXErrorNoValue) {
        std::cerr << indent << "Failed to get title for element. Error: " << titleError << std::endl;
    }

    // Print the identifier if available
    CFTypeRef identifier = nullptr;
    AXError idError = AXUIElementCopyAttributeValue(element, kAXIdentifierAttribute, &identifier);
    if (idError == kAXErrorSuccess && identifier) {
        CFStringRef identifierStr = static_cast<CFStringRef>(identifier);
        char identifierBuffer[256];
        if (CFStringGetCString(identifierStr, identifierBuffer, sizeof(identifierBuffer), kCFStringEncodingUTF8)) {
            std::cout << indent << "Identifier: " << identifierBuffer << std::endl;
        }
        CFRelease(identifier);
    } else if (idError != kAXErrorNoValue) {
        std::cerr << indent << "Failed to get identifier for element. Error: " << idError << std::endl;
    }

    // Get the children of the element
    CFArrayRef children = nullptr;
    AXError childrenError = AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, (CFTypeRef*)&children);
    if (childrenError == kAXErrorSuccess && children) {
        CFIndex count = CFArrayGetCount(children);
        std::cout << indent << "Number of children: " << count << std::endl;

        // Recursively print each child element
        for (CFIndex i = 0; i < count; i++) {
            AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
            CFRetain(child);  // Retain the child to ensure it stays valid
            printChildren(child, level + 1);  // Increase the level for indentation
            CFRelease(child);  // Release the child after use
        }
        CFRelease(children);  // Release the array after use
    } else if (childrenError == kAXErrorInvalidUIElement) {
        std::cerr << indent << "No children or unable to retrieve children for this element. Error: " << childrenError << std::endl;
    } else {
        std::cerr << indent << "Failed to retrieve children. Error: " << childrenError << std::endl;
    }
}

bool LiveInterface::isAnyTextFieldFocusedRecursive(AXUIElementRef parent, int level) {
//    std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

    if (level > 10) {
//        std::cerr << "Max recursion depth reached at level " << level << std::endl;
        return false;
    }

    if (!parent) {
        std::cerr << "Invalid parent element at level " << level << std::endl;
        return false;
    }

    // Initialize children to nullptr
    CFArrayRef children = nullptr;

    // Retrieve the children of the parent element
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
    if (error != kAXErrorSuccess || !children) {
//        std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
        return false;  // Return false if no children found
    }

    CFIndex count = CFArrayGetCount(children);
//    std::cerr << "Number of children at level " << level << ": " << count << std::endl;

    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        if (!child) {
//            std::cerr << "No valid child found at index " << i << std::endl;
            continue;  // Skip to next child if invalid
        }

        // Check if the element is a text field
        CFTypeRef role = nullptr;
        AXError roleError = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, &role);
        if (roleError == kAXErrorSuccess && role) {
            CFStringRef roleStr = static_cast<CFStringRef>(role);
            if (CFStringCompare(roleStr, CFSTR("AXTextField"), 0) == kCFCompareEqualTo) {
                CFRelease(role);

                // Check if the text field is focused
                CFTypeRef focusedValue;
                AXError focusedError = AXUIElementCopyAttributeValue(child, kAXFocusedAttribute, &focusedValue);
                if (focusedError == kAXErrorSuccess && focusedValue == kCFBooleanTrue) {
                    std::cerr << "Focused text field found at level " << level << std::endl;
                    CFRelease(children);
                    return true;  // Exit early if a focused text field is found
                }
                CFRelease(focusedValue);
            } else {
                CFRelease(role);
            }
        }

        // Recursively search in child elements
        if (isAnyTextFieldFocusedRecursive(child, level + 1)) {
            CFRelease(children);  // Release children array before returning
            return true;  // Exit early if a focused text field is found during recursion
        }
    }

    // Release children if nothing is found
    CFRelease(children);
    return false;  // No focused text field found
}

bool LiveInterface::isAnyTextFieldFocused() {
    AXUIElementRef window = findApplicationWindow();
    if (!window) {
        std::cerr << "Window is null or invalid." << std::endl;
        return false;
    }

    return isAnyTextFieldFocusedRecursive(window, 0);
}

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


std::vector<AXUIElementRef> LiveInterface::findElementsByType(AXUIElementRef parent, CFStringRef roleToFind, int level) {
    std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

    std::vector<AXUIElementRef> matches;  // Vector to hold the matches

    if (level > 30) {
        std::cerr << "Max recursion depth reached at level " << level << std::endl;
        return matches;  // Return empty if depth is exceeded
    }

    if (!parent) {
        std::cerr << "Invalid parent element at level " << level << std::endl;
        return matches;
    }

    // Initialize children to nullptr
    CFArrayRef children = nullptr;

    // Retrieve the children of the parent element
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
    if (error != kAXErrorSuccess || !children) {
        std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
        return matches;  // Return empty if no children found
    }

    CFIndex count = CFArrayGetCount(children);
    std::cerr << "Number of children at level " << level << ": " << count << std::endl;

    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        if (!child) {
            std::cerr << "No valid child found at index " << i << std::endl;
            continue;  // Skip to next child if invalid
        }

        std::cerr << "Checking child at index " << i << " - Child element: " << child << std::endl;

        // Check the role (type) of the element
        CFTypeRef role = nullptr;
        AXError roleError = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, &role);
        if (roleError == kAXErrorSuccess && role) {
            CFStringRef roleStr = static_cast<CFStringRef>(role);
            if (CFStringCompare(roleStr, roleToFind, 0) == kCFCompareEqualTo) {
                std::cerr << "Match found at index " << i << " - Level " << level << std::endl;
                CFRetain(child);  // Retain the child before adding it to the vector
                matches.push_back(child);  // Add the match to the vector
                std::cerr << "Child retained and added to matches vector." << std::endl;
            }
            CFRelease(role);  // Always release the role attribute
        }

        // Recursively search in child elements and collect more matches
        std::vector<AXUIElementRef> childMatches = findElementsByType(child, roleToFind, level + 1);
        matches.insert(matches.end(), childMatches.begin(), childMatches.end());  // Append any matches found during recursion
    }

    // Release children array
    CFRelease(children);
    return matches;  // Return the collected matches
}

AXUIElementRef LiveInterface::findElementByAttribute(AXUIElementRef parent, CFStringRef valueToFind, CFStringRef searchAttribute, int level, int maxDepth = 5) {
    std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

    if (level > maxDepth) {
        std::cerr << "Max recursion depth reached at level " << level << std::endl;
        return nullptr;
    }

    if (!parent) {
        std::cerr << "Invalid parent element at level " << level << std::endl;
        return nullptr;
    }

    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
    if (error != kAXErrorSuccess || !children) {
        std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
        return nullptr;
    }

    // Ensure children is released at the end of the function
    std::unique_ptr<const __CFArray, decltype(&CFRelease)> childrenGuard(children, CFRelease);

    CFIndex count = CFArrayGetCount(children);
    std::cerr << "Number of children at level " << level << ": " << count << std::endl;

    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        if (!child) {
            std::cerr << "No valid child found at index " << i << std::endl;
            continue;
        }

        // Get and print the title of each child
        CFStringRef title = nullptr;
        if (AXUIElementCopyAttributeValue(child, kAXTitleAttribute, (CFTypeRef*)&title) == kAXErrorSuccess && title) {
            std::cout << std::string(level * 2, ' ') << "Title: ";
            printCFString(title);
            CFRelease(title);
        }

        std::cerr << "Checking child at index " << i << " - Child element: " << child << std::endl;

        // Check the search attribute (e.g., AXIdentifier or AXRole)
        CFTypeRef attributeValue = nullptr;
        AXError attrError = AXUIElementCopyAttributeValue(child, searchAttribute, &attributeValue);
        if (attrError == kAXErrorSuccess && attributeValue) {
            CFStringRef attrStr = static_cast<CFStringRef>(attributeValue);
            if (CFStringCompare(attrStr, valueToFind, kCFCompareCaseInsensitive | kCFCompareNonliteral) == kCFCompareEqualTo) {
                std::cerr << "Match found at index " << i << " - Level " << level << std::endl;
                CFRelease(attributeValue);  // Release attribute value

                // Return the child immediately
                std::cerr << "Retaining and returning child at level " << level << std::endl;
                CFRetain(child);  // Retain to prevent accidental release
                return child;
            }
            CFRelease(attributeValue);  // Release attribute value if not a match
        }

        // Recursively search in child elements
        std::cerr << "Recursing into child at index " << i << std::endl;
        AXUIElementRef found = findElementByAttribute(child, valueToFind, searchAttribute, level + 1);
        if (found) {
            std::cerr << "Found element during recursion at level " << level << std::endl;
            return found;
        } else {
            std::cerr << "No match found during recursion at level " << level << std::endl;
        }
    }

    std::cerr << "No match found at level " << level << std::endl;
    return nullptr;
}

AXUIElementRef LiveInterface::findElementByIdentifier(AXUIElementRef parent, CFStringRef identifierToFind, int level) {
    std::cerr << "recursion level " << level << " - Parent element: " << parent << std::endl;

    if (level > 5) {
        std::cerr << "Max recursion depth reached at level " << level << std::endl;
        return nullptr;
    }

    if (!parent) {
        std::cerr << "Invalid parent element at level " << level << std::endl;
        return nullptr;
    }

    // Initialize children to nullptr
    CFArrayRef children = nullptr;

    // Retrieve the children of the parent element
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);
    if (error != kAXErrorSuccess || !children) {
        std::cerr << "Failed to retrieve children for the element. Error code: " << error << std::endl;
        return nullptr;  // If failed, return early to avoid accessing nullptr
    }

    CFIndex count = CFArrayGetCount(children);
    std::cerr << "Number of children at level " << level << ": " << count << std::endl;

    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        if (!child) {
            std::cerr << "No valid child found at index " << i << std::endl;
            continue;  // Skip to next child if invalid
        }

        std::cerr << "Checking child at index " << i << " - Child element: " << child << std::endl;

        // Check the identifier
        CFTypeRef identifier = nullptr;
        AXError idError = AXUIElementCopyAttributeValue(child, kAXIdentifierAttribute, &identifier);
        if (idError == kAXErrorSuccess && identifier) {
            CFStringRef identifierStr = static_cast<CFStringRef>(identifier);
            if (CFStringCompare(identifierStr, identifierToFind, 0) == kCFCompareEqualTo) {
                std::cerr << "Match found at index " << i << " - Level " << level << std::endl;
                CFRelease(identifier);  // Release identifier

                // Return the child immediately
                std::cerr << "Retaining and returning child at level " << level << std::endl;
                CFRetain(child);
                CFRelease(children);  // Release children array
                return child;
            }
            CFRelease(identifier);  // Always release identifier if not a match
        }

        // Recursively search in child elements
        std::cerr << "Recursing into child at index " << i << std::endl;
        AXUIElementRef found = findElementByIdentifier(child, identifierToFind, level + 1);
        if (found) {
            std::cerr << "Found element during recursion at level " << level << std::endl;
            CFRelease(children);  // Release children array before returning
            return found;
        } else {
            std::cerr << "No match found during recursion at level " << level << std::endl;
        }
    }

    // Release children if nothing is found
    std::cerr << "Releasing children array at level " << level << std::endl;
    CFRelease(children);
    std::cerr << "No match found at level " << level << std::endl;
    return nullptr;
}

void LiveInterface::searchFocusedTextField(AXUIElementRef parent) {
    if (!parent) {
        std::cerr << "No parent element provided." << std::endl;
        return;
    }

    // Retrieve the children of the parent element
    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(parent, kAXChildrenAttribute, (CFTypeRef*)&children);

    if (error != kAXErrorSuccess || !children) {
        std::cerr << "Unable to retrieve children for the element or no children found." << std::endl;
        return;
    }

    CFIndex count = CFArrayGetCount(children);
    for (CFIndex i = 0; i < count; i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        
        // Check if the child has the AXRoleAttribute
        CFTypeRef role;
        AXError roleError = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, &role);
        if (roleError == kAXErrorSuccess && role) {
            CFStringRef roleStr = static_cast<CFStringRef>(role);

            // Check if the child is a text field (AXTextFieldRole or TAxEditFieldElement)
            if (CFStringCompare(roleStr, kAXTextFieldRole, 0) == kCFCompareEqualTo) {
                // Check if this text field is focused
                CFTypeRef isFocused = nullptr;
                AXError focusedError = AXUIElementCopyAttributeValue(child, kAXFocusedAttribute, &isFocused);
                
                if (focusedError == kAXErrorSuccess && isFocused == kCFBooleanTrue) {
                    std::cout << "Focused text field found" << std::endl;
                    printElementInfo(child);  // Print info about the focused text field
                    CFRelease(role);
                    CFRelease(children);
                    return;
                }
                CFRelease(isFocused);
            }
            CFRelease(role);
        }

        // Recursively search in child elements
        searchFocusedTextField(child);
    }

    CFRelease(children);
}

void LiveInterface::printAllAttributeValues(AXUIElementRef element) {
    CFArrayRef attributeNames = nullptr;
    AXError error = AXUIElementCopyAttributeNames(element, &attributeNames);

    // Handle errors in copying attribute names
    if (error != kAXErrorSuccess || !attributeNames) {
        logger->debug("Failed to retrieve attribute names for the element.");
        return;
    }

    CFIndex count = CFArrayGetCount(attributeNames);
    for (CFIndex i = 0; i < count; i++) {
        CFStringRef attributeName = (CFStringRef)CFArrayGetValueAtIndex(attributeNames, i);
        char nameBuffer[256];
        CFStringGetCString(attributeName, nameBuffer, sizeof(nameBuffer), kCFStringEncodingUTF8);
        logger->debug("Attribute: " + std::string(nameBuffer));

        // Get the attribute value
        CFTypeRef attributeValue = nullptr;
        AXError valueError = AXUIElementCopyAttributeValue(element, attributeName, &attributeValue);
        if (valueError == kAXErrorSuccess && attributeValue != nullptr) {
            // Handle different types of attribute values
            if (CFGetTypeID(attributeValue) == CFStringGetTypeID()) {
                char valueBuffer[256];
                CFStringGetCString((CFStringRef)attributeValue, valueBuffer, sizeof(valueBuffer), kCFStringEncodingUTF8);
                logger->debug("  Value (string): " + std::string(valueBuffer));
            } else if (CFGetTypeID(attributeValue) == CFNumberGetTypeID()) {
                int intValue;
                CFNumberGetValue((CFNumberRef)attributeValue, kCFNumberIntType, &intValue);
                logger->debug("  Value (number): " + std::to_string(intValue));
            } else if (CFGetTypeID(attributeValue) == CFBooleanGetTypeID()) {
                Boolean boolValue = CFBooleanGetValue((CFBooleanRef)attributeValue);
                logger->debug("  Value (boolean): " + std::string(boolValue ? "true" : "false"));
            } else if (CFGetTypeID(attributeValue) == CFArrayGetTypeID()) {
                CFArrayRef arrayValue = (CFArrayRef)attributeValue;
                CFIndex arrayCount = CFArrayGetCount(arrayValue);
                logger->debug("  Value (array), size: " + std::to_string(arrayCount));
                for (CFIndex j = 0; j < arrayCount; j++) {
                    CFTypeRef arrayElement = CFArrayGetValueAtIndex(arrayValue, j);
                    // You can recursively print array elements based on type here, if needed
                }
            } else if (CFGetTypeID(attributeValue) == AXValueGetTypeID()) {
               // AXValueType valueType = AXValueGetType((AXValueRef)attributeValue);
               // if (valueType == kAXValueCGPointType) {
               //     CGPoint point;
               //     AXValueGetValue((AXValueRef)attributeValue, kAXValueCGPointType, &point);
               //     logger->debug("  Value (CGPoint): (" + std::to_string(point.x) + ", " + std::to_string(point.y) + ")");
               // } else if (valueType == kAXValueCGSizeType) {
               //     CGSize size;
               //     AXValueGetValue((AXValueRef)attributeValue, kAXValueCGSizeType, &size);
               //     logger->debug("  Value (CGSize): (" + std::to_string(size.width) + ", " + std::to_string(size.height) + ")");
               // } else {
               //     logger->debug("  Value (AXValue of unknown type)");
               // }
            } else {
                // Handle other types or unknown types
                logger->debug("  Value (unknown type)");
            }

            // Release the attribute value after use
            CFRelease(attributeValue);
        } else {
            logger->debug("  Failed to get value for this attribute");
        }
    }

    // Release the attribute names array
    CFRelease(attributeNames);
}

void LiveInterface::printAllAttributes(const AXUIElementRef element) {
    CFArrayRef attributeNames = nullptr;
    AXError error = AXUIElementCopyAttributeNames(element, &attributeNames);
    if (error == kAXErrorSuccess && attributeNames) {
        CFIndex count = CFArrayGetCount(attributeNames);
        for (CFIndex i = 0; i < count; i++) {
            CFStringRef attributeName = static_cast<CFStringRef>(CFArrayGetValueAtIndex(attributeNames, i));
            char buffer[256];
            if (CFStringGetCString(attributeName, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
                std::cout << "Attribute: " << buffer << std::endl;
            }
        }
        CFRelease(attributeNames);
    } else {
        std::cerr << "Unable to retrieve attributes for this element." << std::endl;
    }
}

void LiveInterface::printFocusedChildElementInfo(const AXUIElementRef mainWindow) {
    if (!mainWindow) {
        std::cerr << "No main window element provided." << std::endl;
        return;
    }

    // Get the focused element in the application (recursively check children)
    AXUIElementRef focusedElement = nullptr;
    AXError error = AXUIElementCopyAttributeValue(mainWindow, kAXFocusedUIElementAttribute, (CFTypeRef*)&focusedElement);

    if (error != kAXErrorSuccess || !focusedElement) {
        std::cerr << "No focused element in the main window." << std::endl;
        return;
    }

    // Print info about the focused element
    printElementInfo(focusedElement);

    // Release the focused element
    CFRelease(focusedElement);
}

// Method to check if the element is focused
bool LiveInterface::isElementFocused(AXUIElementRef windowElement) {
    CFTypeRef isFocused = nullptr;
    AXError result = AXUIElementCopyAttributeValue(windowElement, kAXFocusedAttribute, &isFocused);

    if (result == kAXErrorSuccess && isFocused == kCFBooleanTrue) {
        logger->info("element is focused.");
        if (isFocused != nullptr) {
            CFRelease(isFocused);
        }
        return true;
    }

    logger->info("element is not focused.");
    if (isFocused != nullptr) {
        CFRelease(isFocused);
    }
    return false;
}

// Method to focus the element
void LiveInterface::focusElement(AXUIElementRef element) {
    AXUIElementSetAttributeValue(element, kAXFocusedAttribute, kCFBooleanTrue);
    // TODO error checking
}

// Method to set text in the element
void LiveInterface::setTextInElement(AXUIElementRef element, const char* text) {
    CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
    AXUIElementSetAttributeValue(element, kAXValueAttribute, cfText);
    CFRelease(cfText);
}

bool isPluginWindowTitle(const std::string& title) {
    // This regex matches titles like "aa/2-Audio"
    std::regex pattern(R"(^.*/.*)");
    return std::regex_match(title, pattern);
}

CFStringRef LiveInterface::getRole(AXUIElementRef elementRef) {
    CFStringRef role = nullptr;  // Initialize role to null
    AXError error = AXUIElementCopyAttributeValue(elementRef, kAXRoleAttribute, (CFTypeRef *)&role);

    if (error != kAXErrorSuccess || !role) {
        std::cout << "Failed to get role for element. Error: " << error << std::endl;
        return nullptr;  // Return null if there was an error
    }

    return role;  // Return the role if successful
}

// Method to find and interact with the "Search, text field"
void LiveInterface::findAndInteractWithSearchField() {
    if (!mainWindow_) {
        printf("Failed to get main window for app");
        return;
    }

    CFArrayRef children = nullptr;
    AXError error = AXUIElementCopyAttributeValue(mainWindow_, kAXChildrenAttribute, (CFTypeRef*)&children);
    if (error != kAXErrorSuccess || !children) {
        printf("Failed to get children of main window. Error: %d\n", error);
        return;
    }

    std::unique_ptr<const __CFArray, decltype(&CFRelease)> childrenGuard(children, CFRelease);
    
    for (CFIndex i = 0; i < CFArrayGetCount(children); i++) {
        AXUIElementRef child = (AXUIElementRef)CFArrayGetValueAtIndex(children, i);
        if (!child) continue;

        CFStringRef role = nullptr;
        error = AXUIElementCopyAttributeValue(child, kAXRoleAttribute, (CFTypeRef*)&role);
        if (error != kAXErrorSuccess || !role) continue;

        std::unique_ptr<const __CFString, decltype(&CFRelease)> roleGuard(role, CFRelease);
        
        if (CFStringCompare(role, kAXTextFieldRole, 0) == kCFCompareEqualTo) {
            CFStringRef description = nullptr;
            error = AXUIElementCopyAttributeValue(child, kAXDescriptionAttribute, (CFTypeRef*)&description);
            if (error != kAXErrorSuccess || !description) continue;

            std::unique_ptr<const __CFString, decltype(&CFRelease)> descriptionGuard(description, CFRelease);
            
            if (CFStringCompare(description, CFSTR("Search"), 0) == kCFCompareEqualTo) {
                printf("Found the Search, text field in app\n");

                if (!isElementFocused(child)) {
                    printf("Search field is not focused. Focusing now...\n");
                    focusElement(child);
                } else {
                    printf("Search field is already focused.\n");
                }

                setTextInElement(child, "Hello, world!");
                printf("Text sent to search field.\n");
                return;
            }
        }
    }

    printf("Search field not found\n");
}

