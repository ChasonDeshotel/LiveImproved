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
    //if (self.contextMenu) {
        // If the menu is currently displayed and you want to close it programmatically,
        // you may need to cancel its tracking to ensure the menu fully closes.
        // If tracking isn't canceled and the menu is still open, the menu might remain
        // in an "active" state even though it should be closed
    //    [self.contextMenu cancelTracking];
    //    self.contextMenu = nil;
    //}
    _windowManager()->closeWindow("ContextMenu");
}

- (void)menuItemAction:(id)sender {
    LogHandler::getInstance().debug("Menu item action called");
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
                    LogHandler::getInstance().error("calling handleAction via dispatch: " + actionString);
                    _actionHandler()->handleAction(actionString);
                });
            } else {
                LogHandler::getInstance().error("calling handleAction on main thread: " + actionString);
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
    , menuGenerator_(nil)
    {}

ContextMenu::~ContextMenu() {
    // Clean up the Objective-C object
//    if (menuGenerator_) {
//        [menuGenerator_ release];
//        menuGenerator_ = nil;
//    }
}

void ContextMenu::generateMenu() {
    if (this->menuGenerator_) {
        this->menuGenerator_ = nil;
    }

    auto wm = windowManager_();
    auto ah = actionHandler_();
    if (wm && ah) {
        ContextMenuGenerator *menuGenerator = [[ContextMenuGenerator alloc] initWithContextMenu:this windowManager:windowManager_ actionHandler:actionHandler_];
        if (menuGenerator) {
            this->menuGenerator_ = menuGenerator;

            NSMenu *contextMenu = [menuGenerator createContextMenuWithItems:menuItems_];
            this->contextMenu_ = contextMenu;
            
            NSPoint mouseLocation = [NSEvent mouseLocation];
            [contextMenu popUpMenuPositioningItem:nil atLocation:mouseLocation inView:nil];
            
        } else {
            log_()->error("menuGenerator_ is nullptr!");
            return;
        }
    } else {
        log_()->error("windowManager_ or actionHandler_ is nullptr!");
        return;
    }

}

// NOTE: do not call directly - use WindowManager
void ContextMenu::open() {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            this->generateMenu();
        });
    } else {
        this->generateMenu();
    }
}

// NOTE: do not call directly - use WindowManager
void ContextMenu::close() {
// psure this is needed to call `close` programatically
// but it prevents actions from firing
//    if (contextMenu_) {
//        [contextMenu_ cancelTracking];
//        contextMenu_ = nil;
//    }
}
    
// to appease the interface -- context menus don't have window handles
void* ContextMenu::getWindowHandle() const {
    return nullptr;
}
