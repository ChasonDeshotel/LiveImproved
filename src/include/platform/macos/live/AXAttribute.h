#pragma once

#import <ApplicationServices/ApplicationServices.h>

#include <iostream>
#include <ostream>

#include "LogGlobal.h"

namespace AXAttribute {
    bool isEnabled(AXUIElementRef elem);

    bool isValid(AXUIElementRef element);

    bool isFocused(AXUIElementRef element);

    CFStringRef getRole(AXUIElementRef elementRef);
}
