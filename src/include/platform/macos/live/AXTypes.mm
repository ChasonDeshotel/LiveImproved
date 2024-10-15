#include <string>
#include <ApplicationServices/ApplicationServices.h>

// Helper function to safely cast to CFTypeRef*
inline CFTypeRef* AXValueCast(void* value) {
    // This cast is necessary due to the C-style API of AXUIElementCopyAttributeValue
    return static_cast<CFTypeRef*>(static_cast<void*>(value));
}

std::string axErrorToString(AXError error) {
    switch (error) {
        case kAXErrorSuccess: return "Success";
        case kAXErrorFailure: return "Failure";
        case kAXErrorIllegalArgument: return "Illegal Argument";
        case kAXErrorInvalidUIElement: return "Invalid UI Element";
        case kAXErrorInvalidUIElementObserver: return "Invalid UI Element Observer";
        case kAXErrorCannotComplete: return "Cannot Complete";
        case kAXErrorAttributeUnsupported: return "Attribute Unsupported";
        case kAXErrorActionUnsupported: return "Action Unsupported";
        case kAXErrorNotificationUnsupported: return "Notification Unsupported";
        case kAXErrorNotImplemented: return "Not Implemented";
        case kAXErrorNotificationAlreadyRegistered: return "Notification Already Registered";
        case kAXErrorNotificationNotRegistered: return "Notification Not Registered";
        case kAXErrorAPIDisabled: return "API Disabled";
        case kAXErrorNoValue: return "No Value";
        case kAXErrorParameterizedAttributeUnsupported: return "Parameterized Attribute Unsupported";
        case kAXErrorNotEnoughPrecision: return "Not Enough Precision";
        default: return "Unknown Error";
    }
}
