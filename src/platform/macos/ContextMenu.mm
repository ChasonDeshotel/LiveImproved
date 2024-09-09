#import <Cocoa/Cocoa.h>
#include "ContextMenu.h"
#include "ConfigMenu.h"
#include "Types.h"
#include "LogHandler.h"

static std::function<void(const std::string&)> actionCallback;  // Store the callback

@interface ContextMenuGenerator : NSObject

// Method to create a context menu from a vector of MenuItem and a callback for actions
+ (NSMenu *)createContextMenuWithItems:(const std::vector<MenuItem>&)items
                           actionCallback:(std::function<void(const std::string&)>)callback;

@end


ContextMenu::ContextMenu(std::function<void(const std::string&)> callback)
    : menuItems_()
    , actionCallback_(callback) {

    MenuItem foo;
    foo.label = "new";
    foo.action = "bar";
    MenuItem bar;
    bar.label = "new";
    bar.action = "bar";

    menuItems_ = {
        foo
        , bar
    };

    LogHandler::getInstance().info("Initialized menuItems with size: " + std::to_string(menuItems_.size()));

}

// Open method - shows the menu
void ContextMenu::open() {
    LogHandler::getInstance().info("open called");

    if (!isOpen_) {
        if (![NSThread isMainThread]) {
            LogHandler::getInstance().info("dispatching");
            LogHandler::getInstance().info("menuItems size: " + std::to_string(menuItems_.size()));
            dispatch_async(dispatch_get_main_queue(), ^{
                NSMenu *contextMenu = [ContextMenuGenerator createContextMenuWithItems:menuItems_ actionCallback:actionCallback_];
                NSPoint mouseLocation = [NSEvent mouseLocation];
                [contextMenu popUpMenuPositioningItem:nil atLocation:mouseLocation inView:nil];
            });
            return;
        } else {
            LogHandler::getInstance().info("Already on main thread, creating context menu");
            LogHandler::getInstance().info("menuItems size: " + std::to_string(menuItems_.size()));
            NSMenu *contextMenu = [ContextMenuGenerator createContextMenuWithItems:menuItems_ actionCallback:actionCallback_];
            NSPoint mouseLocation = [NSEvent mouseLocation];
            [contextMenu popUpMenuPositioningItem:nil atLocation:mouseLocation inView:nil];
        }
  
        isOpen_ = true;
    }
}

// Close method - logic to handle closing the menu (if needed)
void ContextMenu::close() {
    if (isOpen_) {
        // Add any specific close logic if required for your context menu
//        if (contextMenu) {
//            LogHandler::getInstance().info("cancel tracking");
//            [contextMenu cancelTracking];
//        }
        isOpen_ = false;
    }
}

// Check if the menu is currently open
bool ContextMenu::isOpen() const {
    return isOpen_;
}

@implementation ContextMenuGenerator

// Recursive helper function to populate the menu with items
+ (void)addMenuItems:(NSMenu *)menu fromItems:(const std::vector<MenuItem>&)items {
    LogHandler::getInstance().info("add item");
    LogHandler::getInstance().info("addMenuItems size: " + std::to_string(items.size()));
    for (const auto& item : items) {
        LogHandler::getInstance().info("item iter");
        if (item.children.empty()) {
            LogHandler::getInstance().info("no children");
            // Regular menu item with an action
            NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:item.label.c_str()]
                                                              action:@selector(menuItemAction:)
                                                       keyEquivalent:@""];
            menuItem.representedObject = [NSString stringWithUTF8String:item.action.c_str()];  // Pass action
            [menuItem setTarget:self];  // Set the target for the action
            [menu addItem:menuItem];
        } else {
            LogHandler::getInstance().info("has children");
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

// Main method to create a context menu with items and a callback for actions
+ (NSMenu *)createContextMenuWithItems:(const std::vector<MenuItem>&)items
                         actionCallback:(std::function<void(const std::string&)>)callback {
    LogHandler::getInstance().info("create menu");
    actionCallback = callback;  // Store the callback function
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Context Menu"];  // Create the menu
    [self addMenuItems:menu fromItems:items];  // Populate the menu
    return menu;
}

// Action triggered by menu items
+ (void)menuItemAction:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    NSString *action = (NSString *)menuItem.representedObject;
    
    // Trigger the callback with the action
    if (actionCallback) {
        actionCallback([action UTF8String]);
    }
}

@end

