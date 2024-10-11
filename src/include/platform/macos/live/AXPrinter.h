#pragma once

#import <ApplicationServices/ApplicationServices.h>

namespace AXPrinter {
    void printAllAttributes(AXUIElementRef element);
    void printAllAttributeValues(AXUIElementRef element);
    void printElementInfo(AXUIElementRef element, std::string prefix = "");
    void printAXElementChildrenRecursively(AXUIElementRef element, int depth = 5, int currentDepth = 0);
    void printAXTree(AXUIElementRef element, int level);
    void printAXTitle(AXUIElementRef elem);
    void printAXIdentifier(AXUIElementRef elem);
    void printAXChildrenInNavigationOrder(AXUIElementRef element);
    void printFocusedElementInChildren(AXUIElementRef parent);
    void printChildren(AXUIElementRef element, int level = 0);
    void printFocusedChildElementInfo(const AXUIElementRef mainWindow);
    void printAllAttributeValues(AXUIElementRef element);
    void printAllAttributes(const AXUIElementRef element);
}
