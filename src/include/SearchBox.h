#ifndef GUI_SEARCH_BOX_H
#define GUI_SEARCH_BOX_H

#include <string>
#include <vector>
#include <memory>

#include "IWindow.h"

class ILogHandler;
class IPluginManager;

#include <JuceHeader.h>

class ApplicationManager;
class PluginManager;
class Plugin;
class EventHandler;
class ActionHandler;
class WindowManager;

class SearchBox : public juce::TopLevelWindow, public IWindow,
                     public juce::TextEditor::Listener, 
                     public juce::ListBoxModel {
public:
    SearchBox(
              std::shared_ptr<ILogHandler> logHandler
              , std::shared_ptr<IPluginManager> pluginManager
              , std::shared_ptr<EventHandler> eventHandler
              , std::shared_ptr<ActionHandler> actionHandler
              , std::shared_ptr<WindowManager> windowManager
        );
    ~SearchBox();

    void open() override;
    void close() override;

    //void setOptions(const std::vector<Plugin>& options);

    //void setSearchText(const std::string& text);
    //std::string getSearchText() const;
    //size_t getSearchTextLength() const;
    //void clearSearchText();

    void textEditorTextChanged(juce::TextEditor& editor) override;
    
    void* getWindowHandle() const override;

protected:
    // JUCE overrides for layout and input handling
    void resized() override;
    void paint(juce::Graphics& g) override;

    // TextEditor listener

    // ListBoxModel methods
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;

private:
    std::shared_ptr<IPluginManager> pluginManager_;
    std::shared_ptr<EventHandler> eventHandler_;
    std::shared_ptr<ActionHandler> actionHandler_;
    std::shared_ptr<WindowManager> windowManager_;

    juce::TextEditor searchField_;
    juce::ListBox listBox_;
    std::unique_ptr<juce::ListBoxModel> pluginListModel_;
    std::vector<Plugin> options_;
    std::vector<Plugin> filteredOptions_;

    void setWindowGeometry();

    //void filterOptions(const juce::String& text);
    void handlePluginSelected(int selectedRow);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchBox)
};

#endif
