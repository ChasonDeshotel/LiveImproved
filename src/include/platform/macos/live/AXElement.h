#pragma once

#import <ApplicationServices/ApplicationServices.h>

#include <iostream>

#include "MacUtils.h"

#include "AXInteraction.h"
#include "AXAttribute.h"
#include "AXPrinter.h"
#include "AXWindow.h"

class AXElement {
protected:
    AXUIElementRef element;

public:
    AXElement(AXUIElementRef elem) : element(elem) {
        if (element) {
            CFRetain(element);
        }
    }

    bool areElementsEqual(AXUIElementRef element1, AXUIElementRef element2) const {
        if (element1 == nullptr || element2 == nullptr) {
            return false;  // Handle null references
        }

        return CFEqual(element1, element2);  // Compare using CFEqual
    }

    // Overload the equality operator
    bool operator==(const AXElement& other) const {
        return areElementsEqual(getRef(), other.getRef());
    }

    bool operator!=(const AXElement& other) const {
        return !areElementsEqual(getRef(), other.getRef());
    }

    virtual AXUIElementRef getRef() const {
        return element;
    }

    virtual void print() {
        AXPrinter::printAllAttributes(element);
    }

    virtual void printTitle() {
        AXPrinter::printAXTitle(element);
    }

    virtual void printValues() {
        AXPrinter::printAllAttributeValues(element);
    }

    virtual bool isEnabled() {
        return AXAttribute::isEnabled(element);
    }

    virtual bool isValid() {
        return AXAttribute::isValid(element);
    }

    virtual bool isFocused() {
        return AXAttribute::isFocused(element);
    }

    virtual bool isPluginWindow() {
        return AXWindow::isPluginWindow(element);
    }

    virtual void focus() {
        AXInteraction::focusElement(element);
    }

    virtual ~AXElement() {
        if (element) {
            CFRelease(element);
        }
    }
};
