#import "NotificationObserver.h"
#import "LiveInterface.h" // Include your C++ header

@implementation NotificationObserver {
    LiveInterface *_interface; // Hold a reference to the C++ object
}

- (instancetype)initWithLiveInterface:(LiveInterface *)interface {
    if (self = [super init]) {
        _interface = interface;
    }
    return self;
}

- (void)windowWillClose:(NSNotification *)notification {
    NSWindow *closingWindow = (NSWindow *)[notification object];
    // Call the C++ method on the LiveInterface instance
    _interface->handleWindowClosure(closingWindow);
}

@end
