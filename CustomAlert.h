#ifndef CustomAlert_H
#define CustomAlert_H

#import <Cocoa/Cocoa.h>

@interface CustomAlert : NSWindowController

@property (nonatomic, assign) BOOL isOpen;

- (instancetype)initWithTitle:(NSString *)title;

- (void)closeAlert;

@end

#endif 
