#pragma once

#import <ApplicationServices/ApplicationServices.h>

#include "LogGlobal.h"

namespace AXCheckBox {
    bool isChecked(AXUIElementRef elem);

    bool toggle(AXUIElementRef checkbox);

    // for closing and re-opening opened plugin windows
    // so that the plugin window order is correct in
    // the main window's AXChildren
    bool toggleOffOn(AXUIElementRef checkbox);
}
