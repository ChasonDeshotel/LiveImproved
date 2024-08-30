#ifndef GUI_SEARCH_BOX_H
#define GUI_SEARCH_BOX_H

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
class GUISearchBox;

// Objective-C class to manage the search box window
@interface GUISearchBoxWindowController : NSWindowController <NSSearchFieldDelegate, NSTableViewDelegate, NSTableViewDataSource>

@property (nonatomic, assign) pid_t livePID;
@property (nonatomic, assign) BOOL isLiveActive;

@property (nonatomic, assign) GUISearchBox *searchBox;
@property (nonatomic, strong) NSSearchField *searchField;
@property (nonatomic, strong) NSArray<NSValue *> *allOptions;
@property (nonatomic, strong) NSMutableArray<NSValue *> *filteredOptions;
@property (nonatomic, strong) NSTableView *resultsTableView;
@property (nonatomic, strong) NSScrollView *tableContainer;
@property (nonatomic, strong) NSVisualEffectView *visualEffectView;

@property (nonatomic, assign) BOOL isOpen;
@property (nonatomic, strong) NSString *searchText;

- (instancetype)initWithTitle:(NSString *)title;
- (void)openAlert;
- (void)closeAlert;
- (void)startMonitoringApplicationFocus;
- (void)applicationDidActivate:(NSNotification *)notification;
- (void)applicationDidDeactivate:(NSNotification *)notification;

@end

#endif

#include <string>
#include <vector>
#include <memory>

#include "Plugin.h"

class ApplicationManager;
class LogHandler;

class GUISearchBox {
public:
    GUISearchBox(ApplicationManager& appManager);
    ~GUISearchBox();

    void initWithTitle(const std::string& title);

    void closeSearchBox();
    void openSearchBox();

    bool isOpen() const;

    void setOptions(const std::vector<Plugin>& options);

    std::string getSearchText() const;
    void setSearchText(const std::string text);
    void clearSearchText();

    void handlePluginSelected(const Plugin& plugin);

    void* getWindowController() const;

private:

    bool isOpen_;

    std::string title;
    std::vector<std::string> allOptions;
    std::vector<std::string> filteredOptions;
    std::shared_ptr<LogHandler> log;

    std::string searchText;

    // placeholder, platform-specific
    void* searchField;
    void* resultsTableView;
    void* tableContainer;
    void* visualEffectView;
    void* windowController_;

};

#endif
