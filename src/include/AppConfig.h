#ifndef APPCONFIG_H
#define APPCONFIG_H

#define JUCE_COMPILER_SUPPORTS_TIME_DATE_MACROS 1

#define JUCE_USE_HARFBUZZ 1

// Include platform-specific headers and JUCE options
#include "juce_core/system/juce_TargetPlatform.h"

// Add other configurations (if needed)
#define JUCE_MODULE_AVAILABLE_juce_core 1
#define JUCE_MODULE_AVAILABLE_juce_events 1
#define JUCE_MODULE_AVAILABLE_juce_graphics 1
#define JUCE_MODULE_AVAILABLE_juce_gui_basics 1

#endif
