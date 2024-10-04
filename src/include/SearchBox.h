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
class IActionHandler;
class WindowManager;
class LimLookAndFeel;
class Theme;

class PluginListModel;

class SearchBox : public juce::TopLevelWindow, public IWindow,
                  public juce::KeyListener,
                  public juce::TextEditor::Listener,
                  public juce::ListBoxModel {
public:
    SearchBox(
              std::function<std::shared_ptr<ILogHandler>()> logHandler
              , std::function<std::shared_ptr<IPluginManager>()> pluginManager
              , std::function<std::shared_ptr<EventHandler>()> eventHandler
              , std::function<std::shared_ptr<IActionHandler>()> actionHandler
              , std::function<std::shared_ptr<WindowManager>()> windowManager
              , std::function<std::shared_ptr<Theme>()> theme
              , std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel
        );
    ~SearchBox();

    void open() override;
    void close() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void mouseDown(const juce::MouseEvent& event) override;

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

private:
    std::function<std::shared_ptr<ILogHandler>()> logHandler_;
    std::function<std::shared_ptr<IPluginManager>()> pluginManager_;
    std::function<std::shared_ptr<EventHandler>()> eventHandler_;
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<WindowManager>()> windowManager_;
    std::function<std::shared_ptr<Theme>()> theme_;
    std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel_;

    juce::TextEditor searchField_;
    juce::ListBox listBox_;
    std::unique_ptr<PluginListModel> pluginListModel_;

    std::vector<Plugin> options_;
    std::vector<Plugin> filteredOptions_;

    void setSelectedRow(int row);
    int selectedRow_;

    void focus();
    void setWindowGeometry();
    void resetFilters();

    //void filterOptions(const juce::String& text);
    void handlePluginSelected();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchBox)
};

#endif
