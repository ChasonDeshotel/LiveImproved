#import "../../PlatformSpecific.h"
#import "GUI.h"
#include "../../LogHandler.h"
#include <string>

@interface CustomAlertWindow : NSWindow
@end

@implementation CustomAlertWindow

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

@end

@implementation GUI
- (instancetype)initWithTitle:(NSString *)title {
    self = [super initWithWindow:nil];
    if (self) {
        //    CGFloat windowWidth = self.window.contentView.frame.size.width;
        NSRect frame = NSMakeRect(0, 0, 400, 400);
        CustomAlertWindow *window = [[CustomAlertWindow alloc] initWithContentRect:frame
                                                       //styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
                                                       styleMask:(NSWindowStyleMaskBorderless)
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        [window setTitle:title];
        [window setBackgroundColor:[NSColor clearColor]];
        [window makeKeyAndOrderFront:nil];
        [self setWindow:window];

        NSRect searchFieldFrame = NSMakeRect(0, 0, 400, 400);
        self.visualEffectView = [[NSVisualEffectView alloc] initWithFrame:searchFieldFrame];
        [self.visualEffectView setWantsLayer:YES];
        self.visualEffectView.layer.cornerRadius = 5.0; 
        self.visualEffectView.layer.masksToBounds = YES;
        
        [self.visualEffectView setMaterial:NSVisualEffectMaterialPopover]; // Choose the material
        [self.visualEffectView setBlendingMode:NSVisualEffectBlendingModeWithinWindow];
        [self.visualEffectView setState:NSVisualEffectStateActive]; // Ensure the effect is active
        
        self.searchField = [[NSSearchField alloc] initWithFrame:NSMakeRect(0, 370, 400, 30)];
        self.searchField.delegate = self;
        [self.searchField setBackgroundColor:[NSColor clearColor]]; // Ensure the background is transparent
        [self.searchField setDrawsBackground:NO];
        [self.visualEffectView addSubview:self.searchField];
        
        [self.window.contentView addSubview:self.visualEffectView];

        self.searchField.recentsAutosaveName = @"CustomSearchFieldRecents";
        self.allOptions = @[@"Option 1", @"Option 2", @"Option 3", @"Another Option", @"More Options"];
        self.filteredOptions = [self.allOptions mutableCopy];
        self.searchField.maximumRecents = 5;

        [self createResultsTableView];

        [window makeFirstResponder:self.searchField];
    }
    return self;
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    [self.window makeFirstResponder:self.searchField];
}

- (void)createResultsTableView {
    self.resultsTableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 240, 400, 100)];
    NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:@"ResultsColumn"];
    [column setTitle:@"Results"];
    [self.resultsTableView addTableColumn:column];
    self.resultsTableView.delegate = self;
    self.resultsTableView.dataSource = self;
    [self.resultsTableView setBackgroundColor:[NSColor clearColor]];
    [self.resultsTableView setGridStyleMask:NSTableViewGridNone];

    self.tableContainer = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 270, 400, 100)];
    [self.tableContainer setDocumentView:self.resultsTableView];
    [self.tableContainer setHasVerticalScroller:YES];
    [self.tableContainer setDrawsBackground:NO];
//    [self.tableContainer setHidden:YES]; // Hide initially

    [self.visualEffectView addSubview:self.tableContainer];
}

- (void)controlTextDidChange:(NSNotification *)notification {
    LogHandler::getInstance().info("control text change");

    self.searchText = [self.searchField stringValue];
    std::string logMessage = "Search text: " + std::string([self.searchText UTF8String]);
    LogHandler::getInstance().info(logMessage);

    if (self.searchText.length == 0) {
        self.filteredOptions = [self.allOptions mutableCopy];
        //[self.tableContainer setHidden:YES];
        LogHandler::getInstance().info("No search text entered. Showing all options.");
    } else {
        NSPredicate *predicate = [NSPredicate predicateWithFormat:@"SELF contains[c] %@", self.searchText];
        self.filteredOptions = [[self.allOptions filteredArrayUsingPredicate:predicate] mutableCopy];
        LogHandler::getInstance().info("Filtered options count: " + std::to_string(self.filteredOptions.count));
        [self.resultsTableView reloadData];
        //[self.tableContainer setHidden:NO];
    }

    [self.resultsTableView reloadData];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.filteredOptions.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    return self.filteredOptions[row];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSInteger selectedRow = [self.resultsTableView selectedRow];
    if (selectedRow >= 0) {
        NSString *selectedOption = self.filteredOptions[selectedRow];
        std::string logMessage = std::string([[NSString stringWithFormat:@"Selected item: %@", selectedOption] UTF8String]);
        LogHandler::getInstance().info(logMessage);
        // do the needful
    }
}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    self.isOpen = YES;
    //[self.window makeFirstResponder:self.searchField];
}

- (void)closeAlert {
		[NSApp stopModal];
    [self close];
    self.isOpen = NO;
}

- (void)close {
		[NSApp stopModal];
    [super close];
    self.isOpen = NO;
    LogHandler::getInstance().info("close called");
}

@end

//- (BOOL)isFuzzyMatch:(NSString *)string withSearchText:(NSString *)self.searchText {
//    NSUInteger searchIndex = 0;
//    NSUInteger stringIndex = 0;
//
//    while (searchIndex < [self.searchText length] && stringIndex < [string length]) {
//        unichar searchChar = [self.searchText characterAtIndex:searchIndex];
//        unichar stringChar = [string characterAtIndex:stringIndex];
//        
//        if ([[NSCharacterSet whitespaceAndNewlineCharacterSet] characterIsMember:searchChar]) {
//            searchIndex++;
//            continue;
//        }
//
//        if ([[NSCharacterSet whitespaceAndNewlineCharacterSet] characterIsMember:stringChar]) {
//            stringIndex++;
//            continue;
//        }
//
//        if ([[NSCharacterSet letterCharacterSet] characterIsMember:searchChar] &&
//            [[NSCharacterSet letterCharacterSet] characterIsMember:stringChar]) {
//            if ([[NSString stringWithFormat:@"%C", searchChar] caseInsensitiveCompare:[NSString stringWithFormat:@"%C", stringChar]] == NSOrderedSame) {
//                searchIndex++;
//            }
//            stringIndex++;
//        } else {
//            if (searchChar == stringChar) {
//                searchIndex++;
//            }
//            stringIndex++;
//        }
//    }
//    
//    return searchIndex == [self.searchText length];
//}
