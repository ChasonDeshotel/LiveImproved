#pragma once

#include <JuceHeader.h>

#include <memory>
#include <vector>

#include "IWindow.h"

class IActionHandler;
class IEventHandler;
class IPluginManager;
class IWindowManager;

class LimLookAndFeel;
class Plugin;
class Theme;

class PluginListModel;

class OverlayComponent : public juce::Component
{
public:
    OverlayComponent()
    {
        // Ensure the component ignores mouse clicks
        setInterceptsMouseClicks(false, false);
    }

    void paint(juce::Graphics& g) override
    {
        // Define the bounds
        auto bounds = getLocalBounds().toFloat();

        // Set the color for the background (or fake corners)
        g.setColour(juce::Colours::orange);  // You can change this

        const float cornerSize = 10.0f;
        juce::Path fakeRoundedCorners;

        // Draw the main rectangle and subtract the corners
        fakeRoundedCorners.addRectangle(bounds);
        fakeRoundedCorners.setUsingNonZeroWinding(false); // Enable subtractive operation
        fakeRoundedCorners.addRoundedRectangle(bounds, cornerSize); // Subtract corners

        // Draw the fake rounded corners or background shape
        g.fillPath(fakeRoundedCorners);
    }

    auto hitTest(int x, int y) -> bool override
    {
        // Let mouse events pass through
        return false;
    }
};


class CenteredTextEditor : public juce::TextEditor {
public:
    CenteredTextEditor() = default;

        void resized() override
    {
        // Define the padding/margins
        const int leftPadding   = 10;
        const int rightPadding  = 10;
        const int topPadding    = 8;
        const int bottomPadding = 8;

        // Calculate the reduced bounds with padding applied
        juce::Rectangle<int> bounds = getLocalBounds();
        bounds.reduce(leftPadding, topPadding);  // Reduces both horizontally and vertically
        bounds.removeFromBottom(bottomPadding);  // Adjust bottom padding
        bounds.removeFromRight(rightPadding);    // Adjust right padding

        // Call the base class resized() to handle other resizing logic
        juce::TextEditor::resized();

        // You can also adjust any child components or internal layout here if necessary
    }
    //void resized() override
    //{
    //    // Calculate the vertical offset without calling setBorder directly
    //    int editorHeight = getHeight();
    //    int textHeight = getFont().getHeight();
    //    int padding = (editorHeight - textHeight) / 2;

    //    // Manually set vertical padding using text offset instead of setBorder
    //    juce::TextEditor::resized();
    //}
};

class SearchBox : public juce::TopLevelWindow, public IWindow,
                  public juce::KeyListener,
                  public juce::TextEditor::Listener,
                  public juce::ListBoxModel {
public:
    SearchBox(
              std::function<std::shared_ptr<IPluginManager>()> pluginManager
              , std::function<std::shared_ptr<IEventHandler>()> eventHandler
              , std::function<std::shared_ptr<IActionHandler>()> actionHandler
              , std::function<std::shared_ptr<IWindowManager>()> windowManager
              , std::function<std::shared_ptr<Theme>()> theme
              , std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel
        );
    ~SearchBox() override;

    SearchBox(SearchBox&&) noexcept = delete;
    SearchBox& operator=(SearchBox&&) noexcept = delete;

    void open() override;
    void close() override;

    auto keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) -> bool override;
    void mouseDown(const juce::MouseEvent& event) override;

    void textEditorTextChanged(juce::TextEditor& editor) override;

    [[nodiscard]] auto getWindowHandle() const -> void* override;

protected:
    // JUCE overrides for layout and input handling
    void resized() override;
    void paint(juce::Graphics& g) override;

    auto getNumRows() -> int override;
    auto paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) -> void override;

private:
    static constexpr int DELAY_BEFORE_FOCUS = 100;
    static constexpr int DELAY_BEFORE_CLOSE = 100;

    static constexpr int WIDGET_WIDTH  = 350;
    static constexpr int WIDGET_HEIGHT = 300;

    static constexpr int ROW_HEIGHT        = 20;
    static constexpr int NUM_VISIBLE_ITEMS = 20;

    static constexpr int PADDING        = 15;
    static constexpr int LISTBOX_OFFSET = 10;

    static constexpr int SEARCHBOX_REMOVE_FROM_TOP = 30;

    std::function<std::shared_ptr<IPluginManager>()> pluginManager_;
    std::function<std::shared_ptr<IEventHandler>()> eventHandler_;
    std::function<std::shared_ptr<IActionHandler>()> actionHandler_;
    std::function<std::shared_ptr<IWindowManager>()> windowManager_;
    std::function<std::shared_ptr<Theme>()> theme_;
    std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel_;

    CenteredTextEditor searchField_;
    juce::ListBox listBox_;
    OverlayComponent overlayComponent_;
    std::unique_ptr<PluginListModel> pluginListModel_;

    std::vector<Plugin> options_;
    std::vector<Plugin> filteredOptions_;

    void setSelectedRow(int row);
    int selectedRow_;

    void focus();
    void setWindowGeometry();
    void resetFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchBox)
};
