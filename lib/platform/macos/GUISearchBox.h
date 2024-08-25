#ifndef GUI_SEARCH_BOX_H
#define GUI_SEARCH_BOX_H

#include <string>
#include <vector>
#include <memory>

#include "../../types/Plugin.h"

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

private:
    ApplicationManager& app_;

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
