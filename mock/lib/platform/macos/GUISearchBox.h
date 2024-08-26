#ifndef GUI_SEARCH_BOX_H
#define GUI_SEARCH_BOX_H

#include <string>
#include <vector>
#include "Plugin.h"

class ApplicationManager;
class LogHandler;

class GUISearchBox {
public:
    GUISearchBox(ApplicationManager& appManager);
    ~GUISearchBox();

    bool isOpen() const;
    std::string getSearchText() const;
    void setSearchText(const std::string text);
    void clearSearchText();
    void openSearchBox();
    void closeSearchBox();
    void setOptions(const std::vector<Plugin>& options);
    void handlePluginSelected(const Plugin& plugin);

private:
    ApplicationManager& app_;
    bool isOpen_;
    std::string searchText_;
    std::vector<Plugin> options_;
};

#endif // GUI_SEARCH_BOX_H

