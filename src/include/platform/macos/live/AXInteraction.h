#pragma once

#import <ApplicationServices/ApplicationServices.h>

#include "LogGlobal.h"

namespace AXInteraction {
    bool focusElement(AXUIElementRef element);
    void closeSpecificWindow(void* windowHandle);
}
