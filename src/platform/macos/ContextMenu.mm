#import <Cocoa/Cocoa.h>
#include "ILogHandler.h"
#include "LogHandler.h"
#include "Types.h"
#include "Utils.h"

#include "IActionHandler.h"
#include "ConfigMenu.h"
#include "ContextMenu.h"
#include "WindowManager.h"

@interface ContextMenuGenerator : NSObject <NSMenuDelegate>

@property (nonatomic, strong) NSMenu *contextMenu;
@property (nonatomic) std::function<void(const std::string&)> overrideCallback;
@property (nonatomic) std::function<std::shared_ptr<WindowManager>()> windowManager;
@property (nonatomic) std::function<std::shared_ptr<IActionHandler>()> actionHandler;
@property (nonatomic, strong) ContextMenuGenerator *menuGenerator;

- (instancetype)initWithContextMenu:(ContextMenu *)contextMenu
    windowManager:(std::function<std::shared_ptr<WindowManager>()>)windowManager
    actionHandler:(std::function<std::shared_ptr<IActionHandler>()>)actionHandler;

- (NSMenu *)createContextMenuWithItems:(const std::vector<MenuItem>&)items;

@end

@implementation ContextMenuGenerator

- (instancetype)initWithContextMenu:(ContextMenu *)contextMenu
    windowManager:(std::function<std::shared_ptr<WindowManager>()>)windowManager
    actionHandler:(std::function<std::shared_ptr<IActionHandler>()>)actionHandler {
    self = [super init];
    if (self) {
        self.windowManager = windowManager;
        self.actionHandler = actionHandler;

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
        LogHandler::getInstance().debug("App became inactive, closing the menu");
        [self closeMenu];
    }
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    if (self.contextMenu) {
        LogHandler::getInstance().debug("App terminating, closing the menu");
        [self closeMenu];
    }
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
    if (self.contextMenu) {
        LogHandler::getInstance().debug("Window minimized, closing the menu");
        [self closeMenu];
    }
}

- (void)windowDidResignKey:(NSNotification *)notification {
    if (self.contextMenu) {
        LogHandler::getInstance().debug("Window lost focus, closing the menu");
        [self closeMenu];
    }
}

- (void)menuDidClose:(NSMenu *)menu {
    if (self.contextMenu) {
        LogHandler::getInstance().debug("Menu closed");
        [self closeMenu];
    }
}

// clean up observers
- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)closeMenu {
//    if (self.contextMenu) {
//        [self.contextMenu cancelTracking];
//        self.contextMenu = nil;
//    }
    _windowManager()->closeWindow("ContextMenu");
}

- (void)menuItemAction:(id)sender {
    LogHandler::getInstance().error("Menu item action called");
    NSMenuItem *menuItem = (NSMenuItem *)sender;
    NSString *action = (NSString *)menuItem.representedObject;
    if (action) {
        if (_actionHandler) {
            std::string actionString = "plugin." + std::string([action UTF8String]);
            Utils::trim(actionString);
            std::vector<std::string> toRemove = {" VST3", " AU", " VST"};
            Utils::removeSubstrings(actionString, toRemove);

            if (![NSThread isMainThread]) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    _actionHandler()->handleAction(actionString);
                });
            } else {
                _actionHandler()->handleAction(actionString);
            }
        } else {
            LogHandler::getInstance().error("Action handler is null");
        }

    } else {
        LogHandler::getInstance().error("Menu item action was nil");
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

- (NSMenu *)createContextMenuWithItems:(const std::vector<MenuItem>&)items {

    if (!self.contextMenu) {
        self.contextMenu = [[NSMenu alloc] initWithTitle:@"Context Menu"];

        // Set self as the delegate to handle menu close events
        [self.contextMenu setDelegate:self];
    }
    [self addMenuItems:self.contextMenu fromItems:items];

    return self.contextMenu;
}

@end

ContextMenu::ContextMenu(
        std::function<std::shared_ptr<ILogHandler>()> logHandler
        , std::function<std::shared_ptr<ConfigMenu>()> configMenu 
        , std::function<std::shared_ptr<IActionHandler>()> actionHandler
        , std::function<std::shared_ptr<WindowManager>()> windowManager
    )
    : log_(std::move(logHandler))
    , configMenu_(std::move(configMenu))
    , actionHandler_(std::move(actionHandler))
    , windowManager_(std::move(windowManager))
    , menuItems_(configMenu_()->getMenuData())
    , menuGenerator_(nullptr)
    {}

ContextMenu::~ContextMenu() {
    #ifdef __OBJC__
        // Clean up the Objective-C object
        if (menuGenerator_) {
            [menuGenerator_ release];
            menuGenerator_ = nil;
        }
    #endif
}

void ContextMenu::generateMenu() {
    #ifdef __OBJC__
    NSLog(@"ContextMenu this pointer: %p", this);

    auto wm = windowManager_();
    auto ah = actionHandler_();
    if (wm) {
        NSLog(@"windowManager_ pointer: %p", wm.get());  // Dereference the shared pointer
    }
    if (ah) {
        NSLog(@"actionHandler_ pointer: %p", ah.get());  // Dereference the shared pointer
    }
    if (wm && ah) {
        menuGenerator_ = [[ContextMenuGenerator alloc] initWithContextMenu:this
                                                             windowManager:windowManager_
                                                             actionHandler:actionHandler_];

    } else {
        NSLog(@"Error: windowManager_ or actionHandler_ is nullptr!");
        return;
    }
    #endif

    if (this->menuGenerator_) {
        NSMenu *contextMenu = [this->menuGenerator_ createContextMenuWithItems:menuItems_];
        this->menuGenerator_ = nil;
        NSPoint mouseLocation = [NSEvent mouseLocation];
        [contextMenu popUpMenuPositioningItem:nil atLocation:mouseLocation inView:nil];
    }

}

// NOTE: do not call directly - use WindowManager
void ContextMenu::open() {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            this->generateMenu();
        });
        return;
    } else {
        this->generateMenu();
    }
}

void ContextMenu::close() {
    log_()->debug("C++ close called");
//    if (menuGenerator_ && menuGenerator_.contextMenu) {
//// TODO: this prevents actions from being fired but is 
//// necessary to close with keyboard
//// add an optional argument i guess?
////        [menuGenerator_.contextMenu cancelTracking];
//    }
}

void* ContextMenu::getWindowHandle() const {
    return nullptr;
}
