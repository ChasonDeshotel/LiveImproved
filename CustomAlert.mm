#import "CustomAlert.h"

@implementation CustomAlert

- (instancetype)initWithTitle:(NSString *)title {
    // Initialize with an empty window
    self = [super initWithWindow:nil];
    if (self) {
				self.isOpen = NO;

        // Create the window
        NSRect frame = NSMakeRect(0, 0, 400, 200);
        NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 400, 200)
                                                       styleMask:(NSWindowStyleMaskBorderless | NSWindowStyleMaskClosable)
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];

        [window setTitle:title];
        [self setWindow:window];

        // Create the text field
        NSTextField *searchField = [[NSTextField alloc] initWithFrame:NSMakeRect(100, 140, 200, 24)];
        [[window contentView] addSubview:searchField];

        // Create the buttons
        NSButton *button1 = [[NSButton alloc] initWithFrame:NSMakeRect(50, 60, 100, 30)];
        [button1 setTitle:@"Option 1"];
        [button1 setTarget:self];
        [button1 setAction:@selector(option1Selected:)];
        [[window contentView] addSubview:button1];

        NSButton *button2 = [[NSButton alloc] initWithFrame:NSMakeRect(160, 60, 100, 30)];
        [button2 setTitle:@"Option 2"];
        [button2 setTarget:self];
        [button2 setAction:@selector(option2Selected:)];
        [[window contentView] addSubview:button2];

        NSButton *button3 = [[NSButton alloc] initWithFrame:NSMakeRect(270, 60, 100, 30)];
        [button3 setTitle:@"Option 3"];
        [button3 setTarget:self];
        [button3 setAction:@selector(option3Selected:)];
        [[window contentView] addSubview:button3];

        // Set the text field as the first responder (focused)
        [window makeFirstResponder:searchField];

        // Add Escape key handling to close the window
        [window setDefaultButtonCell:[button1 cell]]; // Assuming button1 is the default button
    }
    return self;
}

//- (void)keyDown:(NSEvent *)event {
//    if ([event keyCode] == 53) { // 53 is the key code for the Escape key
//        [self close]; // Close the window when Escape is pressed
//    } else {
//        [super keyDown:event]; // Call the default keyDown method for other keys
//    }
//}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    self.isOpen = YES;
}

- (void)closeAlert {
		[NSApp stopModal];
    [self close];
    self.isOpen = NO;
}

// Button actions
- (void)option1Selected:(id)sender {
    NSLog(@"Option 1 selected.");
    [self close];
}

- (void)option2Selected:(id)sender {
    NSLog(@"Option 2 selected.");
    [self close];
}

- (void)option3Selected:(id)sender {
    NSLog(@"Option 3 selected.");
    [self close];
}

@end
