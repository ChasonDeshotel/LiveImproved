#pragma once

#import <ApplicationServices/ApplicationServices.h>

#include "LogGlobal.h"

namespace AXWindow {
    bool isPluginWindow(AXUIElementRef elem);

    CGRect getBounds(AXUIElementRef window);
    void setBounds(AXUIElementRef window, int x, int y, int width, int height);
}
