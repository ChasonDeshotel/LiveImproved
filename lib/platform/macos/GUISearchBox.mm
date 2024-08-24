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
        self.filteredOptions = [NSMutableArray arrayWithArray:self.allOptions];

        NSString *filteredOptionsString = [self.filteredOptions componentsJoinedByString:@", "];
        LogHandler::getInstance().info("filteredOptions: " + std::string([filteredOptionsString UTF8String]));
        [self.resultsTableView reloadData];

    }

    [self.window center];

    return self;
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSString *searchText = [self.searchField stringValue];
    self.searchText = searchText;
    [self.filteredOptions removeAllObjects];

    if ([searchText length] == 0) {
        [self.filteredOptions addObjectsFromArray:self.allOptions];
    } else {
        for (NSString *option in self.allOptions) {
            if ([option rangeOfString:searchText options:NSCaseInsensitiveSearch].location != NSNotFound) {
                [self.filteredOptions addObject:option];
            }
        }
    }

    [self.resultsTableView reloadData];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.filteredOptions.count; // Ensure this returns the correct number of rows
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    return self.filteredOptions[row];
}

- (void)closeAlert {
    [NSApp stopModal];
    [self close];
    self.isOpen = NO;
}

@end

// C++ wrapper
GUISearchBox::GUISearchBox(ApplicationManager& appManager)
    : app_(appManager)
    , title("foo")
    , isOpen_(false)
    , searchText("")
    , windowController_(nullptr)
{

    LogHandler::getInstance().info("Creating GUISearchBoxWindowController");
    NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
    windowController_ = (void*)[[GUISearchBoxWindowController alloc] initWithTitle:nsTitle];

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

std::string GUISearchBox::getSearchText() const {
    if (!windowController_) {
        return "";
    }

    GUISearchBoxWindowController* controller = (GUISearchBoxWindowController*)windowController_;

    NSString* nsSearchText = controller.searchText;
    if (!nsSearchText) {
        return "";
    }

    std::string result([nsSearchText UTF8String]);
    return result;
}

void GUISearchBox::setSearchText(const std::string text) {
    GUISearchBoxWindowController* controller = (GUISearchBoxWindowController*)windowController_;
    NSString* nsSearchText = [NSString stringWithUTF8String:text.c_str()];
    controller.searchText = nsSearchText;
}

void GUISearchBox::clearSearchText() {
    // Clear C++ side
    searchText.clear();

    if (!windowController_) {
        return;
    }

    // Clear the Obj-C side
    GUISearchBoxWindowController* controller = (GUISearchBoxWindowController*)windowController_;
    controller.searchText = @""; // Set to empty string
    LogHandler::getInstance().info("Obj-C searchText cleared");

    // Clear the UI
    [controller.searchField setStringValue:@""];

    // Reset filtered options to show all options
    [controller.filteredOptions removeAllObjects];
    [controller.filteredOptions addObjectsFromArray:controller.allOptions];

    // Reload the table view to display all options
    [controller.resultsTableView reloadData];
    LogHandler::getInstance().info("Table view reloaded");
}

void GUISearchBox::openSearchBox() {
    if (windowController_) {
        CustomAlertWindow* window = (CustomAlertWindow*)[(GUISearchBoxWindowController*)windowController_ window];
        [window setIsVisible:YES];
        [window makeKeyAndOrderFront:nil];
        isOpen_ = true;
    }
}

void GUISearchBox::closeSearchBox() {
    if (windowController_) {
        [(GUISearchBoxWindowController*)windowController_ closeAlert];
        isOpen_ = false;
    }
}

void GUISearchBox::setOptions(const std::vector<std::string>& options) {
    NSMutableArray *nsOptions = [NSMutableArray arrayWithCapacity:options.size()];
    for (const std::string& option : options) {
        NSString *nsString = [NSString stringWithUTF8String:option.c_str()];
        [nsOptions addObject:nsString];
    }

    // set allOptions in the Objective-C class
    GUISearchBoxWindowController *controller = (GUISearchBoxWindowController *)windowController_;
    controller.allOptions = [nsOptions copy];
    controller.filteredOptions = [nsOptions mutableCopy];

    [controller.resultsTableView reloadData];
}
