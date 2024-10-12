#pragma once

#import <ApplicationServices/ApplicationServices.h>

#include "LogGlobal.h"

namespace AXInteraction {
    void focusElement(AXUIElementRef element);
    void closeSpecificWindow(void* windowHandle);
    void closeFocusedPluginWindow();
    void closeAllPlugins();
}
