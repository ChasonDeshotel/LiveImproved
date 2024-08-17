#ifndef CustomAlert_H
#define CustomAlert_H

#import <Cocoa/Cocoa.h>

@interface CustomAlert : NSWindowController

@property (nonatomic, assign) BOOL isOpen;

@property (nonatomic, strong) IBOutlet NSTextField *searchField;

- (instancetype)initWithTitle:(NSString *)title;

- (void)closeAlert;

@end

#endif 
