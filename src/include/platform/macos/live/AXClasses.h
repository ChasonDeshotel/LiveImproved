#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include "AXPrinter.h"
#include "CFStringUtils.h"

class AXElement {
protected:
    AXUIElementRef element;

public:
    AXElement(AXUIElementRef el) : element(el) {}

    virtual void printAttributes() = 0;

    virtual void printAllAttributes() {
        AXPrinter::printAllAttributes(element);
    }

    virtual ~AXElement() {
        if (element) {
            CFRelease(element);
        }
    }
};
