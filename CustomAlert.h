#ifndef CustomAlert_H
#define CustomAlert_H

#import <Cocoa/Cocoa.h>

@interface CustomAlert : NSWindowController <NSSearchFieldDelegate, NSTableViewDelegate, NSTableViewDataSource>

@property (nonatomic, strong) NSSearchField *searchField;
@property (nonatomic, strong) NSArray<NSString *> *allOptions;
@property (nonatomic, strong) NSMutableArray<NSString *> *filteredOptions;
@property (nonatomic, strong) NSTableView *resultsTableView;
@property (nonatomic, strong) NSScrollView *tableContainer;
@property (nonatomic, strong) NSVisualEffectView *visualEffectView;

//@property (nonatomic, strong) NSVisualEffectView *backgroundView;

@property (nonatomic, assign) BOOL isOpen;
@property (nonatomic, strong) NSString *searchText;

- (instancetype)initWithTitle:(NSString *)title;

- (void)closeAlert;

@end

#endif
