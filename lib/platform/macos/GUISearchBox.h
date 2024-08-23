#ifndef GUI_SEARCH_BOX_H
#define GUI_SEARCH_BOX_H

#include <string>
#include <vector>
#include <memory>

class ApplicationManager;
class LogHandler;

class GUISearchBox {
public:
    // Constructor
//    GUI(const std::string& title);
    GUISearchBox(ApplicationManager& appManager);
    ~GUISearchBox();

    void initWithTitle(const std::string& title);

    // Methods
    void closeAlert();
    void showAlert();

    // Getters and Setters
    bool isOpen() const;
    void setIsOpen(bool open);

    void setOptions(const std::vector<std::string>& options);

    const std::string& getSearchText() const;
    void setSearchText(const std::string& text);

private:
    ApplicationManager& app_;

    bool isOpen_;

    // Member variables
    std::string title;
    std::vector<std::string> allOptions;
    std::vector<std::string> filteredOptions;
    std::shared_ptr<LogHandler> log;

    std::string searchText;

    // GUI components (these will be platform-specific in the .cpp or .mm file)
    void* searchField; // Placeholder, platform-specific
    void* resultsTableView; // Placeholder, platform-specific
    void* tableContainer; // Placeholder, platform-specific
    void* visualEffectView; // Placeholder, platform-specific
    void* windowController_;
};

#endif
