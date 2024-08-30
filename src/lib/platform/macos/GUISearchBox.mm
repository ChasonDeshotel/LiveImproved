#import <Cocoa/Cocoa.h>

#include "LogHandler.h"
#include "ApplicationManager.h"
#include "GUISearchBox.h"
#include "Plugin.h"
#include "PID.h"

// Custom window class to allow the window to become key and main
@interface CustomAlertWindow : NSWindow
@end


// TODO: custom table view to highlight the row
// and allow keyboard navigation while still
// putting text to the box
@interface CustomTableView : NSTableView
@end

@implementation CustomTableView

- (void)keyDown:(NSEvent *)event {
    NSString *characters = [event characters];
    if ([characters length] > 0 && [[characters stringByTrimmingCharactersInSet:[NSCharacterSet controlCharacterSet]] length] > 0) {
        // If the key is a text character, send it to the search field
        [[self.window firstResponder] keyDown:event];
    } else {
        // Otherwise, handle it as usual
        [super keyDown:event];
    }
}

@end

@implementation CustomAlertWindow

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

- (void)performClose:(id)sender {
    [super performClose:sender];
    [NSApp stopModal];  // Ensure the modal session ends when the window is closed
}

@end

@implementation GUISearchBoxWindowController

- (void)keyDown:(NSEvent *)event {
    [super keyDown:event];
    // Handle specific key events
    LogHandler::getInstance().info("Key Down");
}

- (void)keyUp:(NSEvent *)event {
    [super keyUp:event];
    // Handle specific key events
    LogHandler::getInstance().info("Key Up");
}

- (void)runModalLoopForWindow:(NSWindow *)window {
    while (self.isOpen) {
        NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                             untilDate:[NSDate distantFuture]
                                                inMode:NSDefaultRunLoopMode
                                               dequeue:YES];
        
        [NSApp sendEvent:event];
        [NSApp updateWindows];
        
        if ([event type] == NSEventTypeKeyDown || [event type] == NSEventTypeKeyUp) {
            NSNotification *notification = [NSNotification notificationWithName:NSTextDidChangeNotification object:self.searchField];
            [self controlTextDidChange:notification];  // Trigger text change event manually
        }
    }
}


- (instancetype)initWithTitle:(NSString *)title {
    LogHandler::getInstance().info("init with title called");
    self = [super initWithWindow:nil];
    if (self) {

        pid_t livePID = PID::getInstance().livePID();
        self.isLiveActive = YES;

        NSRect frame = NSMakeRect(0, 0, 400, 400);
        
        // Use CustomAlertWindow to ensure the window can accept input
        CustomAlertWindow *window = [[CustomAlertWindow alloc] initWithContentRect:frame
                                                                         styleMask:NSWindowStyleMaskBorderless
                                                                           backing:NSBackingStoreBuffered
                                                                             defer:NO];
        [window setTitle:title];
        [window setBackgroundColor:[NSColor clearColor]];

        // always on top
        [window setLevel:NSFloatingWindowLevel];

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

        [self startMonitoringApplicationFocus];

    }

    [self.window center];

    return self;
}


- (void)startMonitoringApplicationFocus {
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                           selector:@selector(applicationDidActivate:)
                                                               name:NSWorkspaceDidActivateApplicationNotification
                                                             object:nil];

    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                           selector:@selector(applicationDidDeactivate:)
                                                               name:NSWorkspaceDidDeactivateApplicationNotification
                                                             object:nil];
}

- (void)applicationDidActivate:(NSNotification *)notification {
    NSDictionary *userInfo = [notification userInfo];
    NSRunningApplication *runningApp = [userInfo objectForKey:NSWorkspaceApplicationKey];
    pid_t pid = runningApp.processIdentifier;
    pid_t livePID = PID::getInstance().livePID();

    if (pid == self.livePID && !self.isLiveActive && self.isOpen) {
        CustomAlertWindow* window = (CustomAlertWindow*)[self window];
        LogHandler::getInstance().info("did activate. pid: " + std::to_string(pid)
          + " live pid: " + std::to_string(self.livePID));
        [window setIsVisible:YES];
        [window makeKeyAndOrderFront:nil];
        self.isLiveActive = YES;
    }
}

- (void)applicationDidDeactivate:(NSNotification *)notification {
    NSDictionary *userInfo = [notification userInfo];
    NSRunningApplication *runningApp = [userInfo objectForKey:NSWorkspaceApplicationKey];
    pid_t pid = runningApp.processIdentifier;
    pid_t livePID = PID::getInstance().livePID();

    if (pid == self.livePID && self.isLiveActive && self.isOpen) {
        CustomAlertWindow* window = (CustomAlertWindow*)[self window];
        LogHandler::getInstance().info("did deactivate. pid: " + std::to_string(pid)
        + " live pid: " + std::to_string(self.livePID));
        [window orderOut:nil];  // Hides the window
        self.isLiveActive = NO;
    }
}

- (void)controlTextDidChange:(NSNotification *)notification {
    if (notification == nil) {
        // Handle the case where notification is nil
        return;
    }
    
    NSString *searchText = [self.searchField stringValue];
    self.searchText = searchText;
    [self.filteredOptions removeAllObjects];

    if ([searchText length] == 0) {
        [self.filteredOptions addObjectsFromArray:self.allOptions];
    } else {
        for (NSValue *pluginValue in self.allOptions) {
            Plugin *plugin = (Plugin *)[pluginValue pointerValue];
            NSString *pluginName = [NSString stringWithUTF8String:plugin->name.c_str()];
            if ([pluginName rangeOfString:searchText options:NSCaseInsensitiveSearch].location != NSNotFound) {
                [self.filteredOptions addObject:pluginValue];
            }
        }
    }

    [self.resultsTableView reloadData];

    if (self.filteredOptions.count == 1) {
        [self.resultsTableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
        NSNotification *dummyNotification = [NSNotification notificationWithName:NSTableViewSelectionDidChangeNotification object:self.resultsTableView];
        [self tableViewSelectionDidChange:dummyNotification]; // Trigger the selection change manually
    }
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSTableView *tableView = (NSTableView *)[notification object];
    NSInteger selectedRow = [tableView selectedRow];

    if (selectedRow >= 0) {
        // Access the corresponding Plugin object
        NSValue *pluginValue = [self.filteredOptions objectAtIndex:selectedRow];
        Plugin *selectedPlugin = (Plugin *)[pluginValue pointerValue];

        if (self.filteredOptions.count == 1) {
            // Access the corresponding Plugin object
          LogHandler::getInstance().info("Selected plugin: " + selectedPlugin->name);
            NSValue *pluginValue = [self.filteredOptions objectAtIndex:selectedRow];
            Plugin *selectedPlugin = (Plugin *)[pluginValue pointerValue];
            if (self.searchBox) {
                self.searchBox->handlePluginSelected(*selectedPlugin);
            }
        }
    }
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.filteredOptions.count; // Ensure this returns the correct number of rows
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if (row < self.filteredOptions.count) {
        NSValue *pluginValue = [self.filteredOptions objectAtIndex:row];
        Plugin *plugin = (Plugin *)[pluginValue pointerValue];
        return [NSString stringWithUTF8String:plugin->name.c_str()];
    }
    return nil;
}

- (void)openAlert {
    if (self.isOpen) {
        LogHandler::getInstance().info("Search box is already open");
        return;
    }

    LogHandler::getInstance().info("Opening the search box");

    CustomAlertWindow* window = (CustomAlertWindow*)[self window];
    
    [window setLevel:NSModalPanelWindowLevel];  // Keep the window on top
    [window setHidesOnDeactivate:NO];  // Ensure the window stays active even when the app is not focused
    [window setIsVisible:YES];  // Make the window visible
    [window makeKeyAndOrderFront:nil];  // Bring the window to the front
    [window makeFirstResponder:window.contentView];
    [window orderFrontRegardless]; // if the window's been moved to the back, pop it back to the front

    self.isOpen = YES;  // Update the state

    [NSApp runModalForWindow:window];  // Start the modal session
    [self performSelectorOnMainThread:@selector(runModalLoopForWindow:) withObject:window waitUntilDone:NO];


    ApplicationManager::getInstance().getEventHandler()->focusApplication(PID::getInstance().appPID());
}

- (void)closeAlert {
    if (self.isOpen) {
        LogHandler::getInstance().info("Closing the search box");

        [NSApp stopModal]; // Stop the modal session
        
        CustomAlertWindow* window = (CustomAlertWindow*)[self window];
        if (window) {
            [window orderOut:nil]; // Hide the window
        }
        
        self.isOpen = NO;

        ApplicationManager::getInstance().getEventHandler()->focusApplication(PID::getInstance().livePID());
    } else {
        LogHandler::getInstance().info("Search box is already closed");
    }
}

@end

// C++ wrapper
GUISearchBox::GUISearchBox(ApplicationManager& appManager)
    : title("foo")
    , isOpen_(false)
    , searchText("")
    , windowController_(nullptr)
{

    LogHandler::getInstance().info("Creating GUISearchBoxWindowController");
    NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
    windowController_ = (void*)[[GUISearchBoxWindowController alloc] initWithTitle:nsTitle];

    // Pass the C++ object pointer to the Objective-C controller
    GUISearchBoxWindowController* controller = (GUISearchBoxWindowController*)windowController_;
    controller.searchBox = this;
    //((GUISearchBoxWindowController*)windowController_).searchBox = this;

//    pid_t livePID = ApplicationManager::getInstance().getPID();
//    controller.livePID = livePID;

//    LogHandler::getInstance().info("GUISearchBox::GUISearchBox - Retrieved live PID: " + std::to_string(livePID));

//    ((GUISearchBoxWindowController*)windowController_).searchBox = this;

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

void* GUISearchBox::getWindowController() const {
    return windowController_;
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
    if (isOpen_) {
        LogHandler::getInstance().info("Search box is already open");
        return;
    }

    if (windowController_) {
        isOpen_ = true;  // Set the flag to true before proceeding

        // Call the Objective-C method to handle the opening of the search box
        [(GUISearchBoxWindowController*)windowController_ performSelectorOnMainThread:@selector(openAlert) withObject:nil waitUntilDone:NO];
    } else {
        LogHandler::getInstance().error("windowController_ is null, cannot open search box");
        isOpen_ = false;  // Reset the flag if opening fails
    }
}

void GUISearchBox::closeSearchBox() {
    if (!isOpen_) {
        LogHandler::getInstance().info("Search box is not open");
        return;
    }

    if (windowController_) {
        GUISearchBoxWindowController* controller = (GUISearchBoxWindowController*)windowController_;
        [controller performSelectorOnMainThread:@selector(closeAlert) withObject:nil waitUntilDone:YES];

        isOpen_ = false;
        controller.isOpen = NO;

    } else {
        LogHandler::getInstance().error("windowController_ is null, cannot close search box");
    }
}

void GUISearchBox::setOptions(const std::vector<Plugin>& options) {
    NSMutableArray *nsPlugins = [NSMutableArray arrayWithCapacity:options.size()];

    for (const Plugin& plugin : options) {
        // Store the pointer to the Plugin object in an NSValue
        NSValue *pluginValue = [NSValue valueWithPointer:&plugin];
        [nsPlugins addObject:pluginValue];
    }

    // Set allOptions in the Objective-C class
    GUISearchBoxWindowController *controller = (GUISearchBoxWindowController *)windowController_;
    controller.allOptions = [nsPlugins copy];
    controller.filteredOptions = [nsPlugins mutableCopy];

    [controller.resultsTableView reloadData];
}

void GUISearchBox::handlePluginSelected(const Plugin& plugin) {
    // Handle the selected Plugin object on the C++ side
    LogHandler::getInstance().info("Plugin selected: " + plugin.name);
    // Perform additional actions as needed
}
