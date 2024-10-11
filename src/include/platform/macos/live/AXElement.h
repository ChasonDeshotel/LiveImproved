#pragma once

#import <ApplicationServices/ApplicationServices.h>

#include <iostream>

#include "MacUtils.h"

#include "AXAction.h"
#include "AXAttribute.h"
#include "AXPrinter.h"

class AXElement {
protected:
    AXUIElementRef element;

public:
    AXElement(AXUIElementRef el) : element(el) {}

    virtual AXUIElementRef getRef() {
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

    virtual void focus() {
        AXAction::focusElement(element);
    }

    virtual ~AXElement() {
        if (element) {
            CFRelease(element);
        }
    }
};
