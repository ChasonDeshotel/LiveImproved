#ifndef GUI_SEARCH_BOX_H
#define GUI_SEARCH_BOX_H

#include <string>
#include <vector>
#include <memory>

#include "Types.h" // for IWindow

class LogHandler;

#include <JuceHeader.h>

class ApplicationManager;
class PluginManager;

class SearchBox : public juce::TopLevelWindow, public IWindow,
                     public juce::TextEditor::Listener, 
                     public juce::ListBoxModel {
public:
    SearchBox();
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
    ApplicationManager& app_;
    PluginManager& pluginManager_;

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
