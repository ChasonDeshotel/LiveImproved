#import <Foundation/Foundation.h>

class LiveInterface; // Forward declare your C++ class

@interface NotificationObserver : NSObject

- (instancetype)initWithLiveInterface:(LiveInterface *)interface;

@end
