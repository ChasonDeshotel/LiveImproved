#import "GUISearchBox.h"
#import "../../ApplicationManager.h"
#import <Cocoa/Cocoa.h>
#include "../../LogHandler.h"

// Custom window class to allow the window to become key and main
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

// Objective-C class to manage the search box window
@interface GUISearchBoxWindowController : NSWindowController <NSSearchFieldDelegate, NSTableViewDelegate, NSTableViewDataSource>

@property (nonatomic, strong) NSSearchField *searchField;
@property (nonatomic, strong) NSArray<NSString *> *allOptions;
@property (nonatomic, strong) NSMutableArray<NSString *> *filteredOptions;
@property (nonatomic, strong) NSTableView *resultsTableView;
@property (nonatomic, strong) NSScrollView *tableContainer;
@property (nonatomic, strong) NSVisualEffectView *visualEffectView;

@property (nonatomic, assign) LogHandler* log;

@property (nonatomic, assign) BOOL isOpen;
@property (nonatomic, strong) NSString *searchText;

- (instancetype)initWithTitle:(NSString *)title;
- (void)closeAlert;

@end

@implementation GUISearchBoxWindowController

- (instancetype)initWithTitle:(NSString *)title {
    LogHandler::getInstance().info("init with title called");
    self = [super initWithWindow:nil];
    if (self) {
        NSRect frame = NSMakeRect(0, 0, 400, 400);
        
        // Use CustomAlertWindow to ensure the window can accept input
        CustomAlertWindow *window = [[CustomAlertWindow alloc] initWithContentRect:frame
                                                                         styleMask:NSWindowStyleMaskBorderless
                                                                           backing:NSBackingStoreBuffered
                                                                             defer:NO];
        [window setTitle:title];
        [window setBackgroundColor:[NSColor clearColor]];

        // always on top
        //[window setLevel:NSFloatingWindowLevel];

        [self setWindow:window];

        NSRect searchFieldFrame = NSMakeRect(0, 0, 400, 400);
        self.visualEffectView = [[NSVisualEffectView alloc] initWithFrame:searchFieldFrame];
        [self.visualEffectView setWantsLayer:YES];
        self.visualEffectView.layer.cornerRadius = 5.0;
        self.visualEffectView.layer.masksToBounds = YES;

        [self.visualEffectView setMaterial:NSVisualEffectMaterialPopover];
        [self.visualEffectView setBlendingMode:NSVisualEffectBlendingModeWithinWindow];
        [self.visualEffectView setState:NSVisualEffectStateActive];

        self.searchField = [[NSSearchField alloc] initWithFrame:NSMakeRect(0, 370, 400, 30)];
        self.searchField.delegate = self;
        [self.searchField setBackgroundColor:[NSColor clearColor]];
        [self.searchField setDrawsBackground:NO];

        [self.visualEffectView addSubview:self.searchField];

        NSRect tableViewFrame = NSMakeRect(0, 0, 400, 340);
        self.resultsTableView = [[NSTableView alloc] initWithFrame:tableViewFrame];
        
        NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:@"ResultsColumn"];
        [column setWidth:400];
        [[column headerCell] setStringValue:@"Results"];
        [self.resultsTableView addTableColumn:column];
        
        self.resultsTableView.delegate = self;
        self.resultsTableView.dataSource = self;

        self.tableContainer = [[NSScrollView alloc] initWithFrame:tableViewFrame];
        [self.tableContainer setDocumentView:self.resultsTableView];
        [self.tableContainer setHasVerticalScroller:YES];
        [self.tableContainer setAutohidesScrollers:YES];
        [self.tableContainer setBorderType:NSNoBorder];

        [self.visualEffectView addSubview:self.tableContainer];

        [[window contentView] addSubview:self.visualEffectView];

        self.searchField.recentsAutosaveName = @"CustomSearchFieldRecents";
        self.allOptions = @[@"Option 1", @"Option 2", @"Option 3", @"Another Option", @"More Options"];
        self.filteredOptions = [NSMutableArray arrayWithArray:self.allOptions];

    }

    //[[self window] orderOut:nil];
    [self.window center];
    //[self.window makeKeyAndOrderFront:nil];

    return self;
}

//- (void)showWindow:(id)sender {
//    [super showWindow:sender];
////    dispatch_async(dispatch_get_main_queue(), ^{
////        [NSApp runModalForWindow:[self window]];
////    });
//
//    self.isOpen = YES;
//    //[self.window makeFirstResponder:self.searchField];
//}
//
- (void)closeAlert {
    [NSApp stopModal];
    [self close];
    self.isOpen = NO;
}

@end

// C++ wrapper
GUISearchBox::GUISearchBox(ApplicationManager& appManager)
    : app_(appManager), title("foo"), isOpen_(false), windowController_(nullptr) {
    // Instantiate the Objective-C class
    LogHandler::getInstance().info("Creating GUISearchBoxWindowController");

    NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
    windowController_ = (void*)[[GUISearchBoxWindowController alloc] initWithTitle:nsTitle];

    // Ensure that all UI components are initialized
    if (windowController_) {
        LogHandler::getInstance().info("Successfully created GUISearchBoxWindowController");
    } else {
        LogHandler::getInstance().error("Failed to create GUISearchBoxWindowController");
    }
}

GUISearchBox::~GUISearchBox() {
    if (windowController_) {
        [(GUISearchBoxWindowController*)windowController_ closeAlert];
        windowController_ = nullptr;
    }
}

bool GUISearchBox::isOpen() const {
    return isOpen_;
}

void GUISearchBox::setIsOpen(bool open) {
    isOpen_ = open;
}

const std::string& GUISearchBox::getSearchText() const {
    return searchText;
}

void GUISearchBox::setSearchText(const std::string& text) {
    searchText = text;
}

void GUISearchBox::showAlert() {
    if (windowController_) {
        CustomAlertWindow* window = (CustomAlertWindow*)[(GUISearchBoxWindowController*)windowController_ window];
        LogHandler::getInstance().info("showAlert called - making window visible");
        [window setIsVisible:YES];  // Explicitly show the window
        [window makeKeyAndOrderFront:nil];  // Bring it to the front
        isOpen_ = true;
    }
}

void GUISearchBox::closeAlert() {
    if (windowController_) {
        [(GUISearchBoxWindowController*)windowController_ closeAlert];
        isOpen_ = false;
    }
}


//void GUISearchBox::initWithTitle(const std::string& title) {
//    if (!windowController_) {
//        LogHandler::getInstance().info("Initializing GUISearchBoxWindowController");
//        NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
//        windowController_ = (void*)[[GUISearchBoxWindowController alloc] initWithTitle:nsTitle];
//        GUISearchBoxWindowController* controller = (GUISearchBoxWindowController*)windowController_;
////        windowController_ = [[GUISearchBoxWindowController alloc] initWithTitle:nsTitle];
//        searchField = (void *)controller.searchField;
//        visualEffectView = (void *)controller.visualEffectView;
//// hide on launch
////        [controller showWindow:nil];
//    }
//}
