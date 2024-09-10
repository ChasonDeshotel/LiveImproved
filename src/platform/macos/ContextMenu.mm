#import <Cocoa/Cocoa.h>
#include "ApplicationManager.h"
#include "ContextMenu.h"
#include "ConfigMenu.h"
#include "Types.h"
#include "LogHandler.h"

@interface ContextMenuGenerator : NSObject <NSMenuDelegate>

@property (nonatomic, strong) NSMenu *contextMenu;
@property (nonatomic) std::function<void(const std::string&)> overrideCallback;

- (instancetype)initWithContextMenu:(ContextMenu *)contextMenu;

- (NSMenu *)createContextMenuWithItems:(const std::vector<MenuItem>&)items
                         overrideCallback:(std::function<void(const std::string&)>)callback;

@end

@implementation ContextMenuGenerator

- (instancetype)initWithContextMenu:(ContextMenu *)contextMenu {
    self = [super init];
    if (self) {
        // Register for app-related notifications
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(applicationDidResignActive:)
                                                     name:NSApplicationDidResignActiveNotification
                                                   object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(applicationWillTerminate:)
                                                     name:NSApplicationWillTerminateNotification
                                                   object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidMiniaturize:)
                                                     name:NSWindowDidMiniaturizeNotification
                                                   object:nil];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidResignKey:)
                                                     name:NSWindowDidResignKeyNotification
                                                   object:nil];
    }
    return self;
}

- (void)applicationDidResignActive:(NSNotification *)notification {
    if (self.contextMenu) {
        LogHandler::getInstance().info("App became inactive, closing the menu");
        [self closeMenu];
    }
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    if (self.contextMenu) {
        LogHandler::getInstance().info("App terminating, closing the menu");
        [self closeMenu];
    }
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
    if (self.contextMenu) {
        LogHandler::getInstance().info("Window minimized, closing the menu");
        [self closeMenu];
    }
}

- (void)windowDidResignKey:(NSNotification *)notification {
    if (self.contextMenu) {
        LogHandler::getInstance().info("Window lost focus, closing the menu");
        [self closeMenu];
    }
}

- (void)menuDidClose:(NSMenu *)menu {
    if (self.contextMenu) {
        LogHandler::getInstance().info("Menu closed");
        [self closeMenu];
    }
}

- (void)closeMenu {
//    if (self.contextMenu) {
//        [self.contextMenu cancelTracking];
//        self.contextMenu = nil;
//    }
    ApplicationManager::getInstance().getWindowManager()->closeWindow("ContextMenu");
}

- (void)menuItemAction:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    NSString *action = (NSString *)menuItem.representedObject;
    
    if (self.overrideCallback) {
        self.overrideCallback([action UTF8String]);
    } else {
        std::string actionString = [action UTF8String];
        ApplicationManager::getInstance().getActionHandler()->handleAction(actionString);
    }
}

- (void)addMenuItems:(NSMenu *)menu fromItems:(const std::vector<MenuItem>&)items {
//    LogHandler::getInstance().info("addMenuItems size: " + std::to_string(items.size()));
    for (const auto& item : items) {
        if (item.children.empty()) {
            NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:item.label.c_str()]
                                                              action:@selector(menuItemAction:)
                                                       keyEquivalent:@""];

            menuItem.representedObject = [NSString stringWithUTF8String:item.action.c_str()];
            [menuItem setTarget:self];
            [menu addItem:menuItem];

        } else {
            // Submenu
            NSMenuItem *submenuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:item.label.c_str()]
                                                                 action:nil
                                                          keyEquivalent:@""];

            NSMenu *submenu = [[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:item.label.c_str()]];
            [self addMenuItems:submenu fromItems:item.children];  // Recursively add children
            [submenuItem setSubmenu:submenu];
            [menu addItem:submenuItem];
        }
    }
}

- (NSMenu *)createContextMenuWithItems:(const std::vector<MenuItem>&)items
                         overrideCallback:(std::function<void(const std::string&)>)callback {

    self.overrideCallback = callback;

    if (!self.contextMenu) {
        self.contextMenu = [[NSMenu alloc] initWithTitle:@"Context Menu"];

        // Set self as the delegate to handle menu close events
        [self.contextMenu setDelegate:self];
    }
    [self addMenuItems:self.contextMenu fromItems:items];

    return self.contextMenu;
}

@end

ContextMenu::ContextMenu(std::function<void(const std::string&)> callback)
    : menuItems_()
    , overrideCallback_(callback) {

    menuItems_ = ApplicationManager::getInstance().getConfigMenu()->getMenuData();
}

// NOTE: do not call directly - use WindowManager
void ContextMenu::open() {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            ContextMenuGenerator *menuGenerator = [[ContextMenuGenerator alloc] init];
            this->menuGenerator_ = menuGenerator;
            NSMenu *contextMenu = [menuGenerator createContextMenuWithItems:menuItems_ 
                                                            overrideCallback:overrideCallback_];
            NSPoint mouseLocation = [NSEvent mouseLocation];
            [contextMenu popUpMenuPositioningItem:nil atLocation:mouseLocation inView:nil];
        });
        return;

    } else {
        ContextMenuGenerator *menuGenerator = [[ContextMenuGenerator alloc] init];
        this->menuGenerator_ = menuGenerator;
        NSMenu *contextMenu = [menuGenerator createContextMenuWithItems:menuItems_ 
                                                        overrideCallback:overrideCallback_];
        NSPoint mouseLocation = [NSEvent mouseLocation];
        [contextMenu popUpMenuPositioningItem:nil atLocation:mouseLocation inView:nil];
    }
}

void ContextMenu::close() {
    LogHandler::getInstance().info("C++ close called");
    if (menuGenerator_ && menuGenerator_.contextMenu) {
// TODO: this prevents actions from being fired but is 
// necessary to close with keyboard
// add an optional argument i guess?
//        [menuGenerator_.contextMenu cancelTracking];
    }
}
