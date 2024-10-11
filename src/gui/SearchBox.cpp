#define NOMINMAX
#include <JuceHeader.h>
#include <algorithm>
#include <functional>

#include "LogGlobal.h"
#include "Types.h"

#include "EventHandler.h"
#include "IActionHandler.h"
#include "LimLookAndFeel.h"
#include "PluginManager.h"
#include "SearchBox.h"
#include "Theme.h"
#include "WindowManager.h"

class PluginListModel : public juce::ListBoxModel {
public:
    PluginListModel(std::shared_ptr<IPluginManager> pluginManager, std::shared_ptr<IActionHandler> actionHandler, std::shared_ptr<WindowManager> windowManager, std::shared_ptr<Theme> theme)
        : pluginManager_(std::move(pluginManager))
        , actionHandler_(std::move(actionHandler))
        , windowManager_(std::move(windowManager))
        , theme_(std::move(theme))
        , plugins_(pluginManager_->getPlugins())
        , filteredPlugins_(plugins_)
    {}

    int getNumRows() override {
        return static_cast<int>(filteredPlugins_.size());
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override {
//        juce::Graphics::ScopedSaveState saveState(g);
//
//        // Extend the clipping region to include the scrollbar area
//        g.reduceClipRegion(-18, 0, width + 18, height);

        if (rowIsSelected) {
            g.fillAll(theme_->getColorValue("SelectionBackground"));
            g.setColour(theme_->getColorValue("SelectionForeground"));
        } else {
            g.fillAll(juce::Colours::transparentBlack);
            g.setColour(theme_->getColorValue("ControlForeground"));
        }
        if (rowNumber < filteredPlugins_.size()) {
            auto& plugin = filteredPlugins_[rowNumber];
            g.drawText(plugin.name, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override {
        if (row >= 0 && row < filteredPlugins_.size()) {
            const Plugin* plugin = getPluginAtRow(row);
            if (plugin) {
                int pluginID = plugin->number;
                juce::Timer::callAfterDelay(100, [this, pluginID]() {
                    actionHandler_->loadItem(pluginID);
                    windowManager_->closeWindow("SearchBox");
                });
            }
        }
    }

    const Plugin* getPluginAtRow(int row) const {
        if (row >= 0 && row < filteredPlugins_.size()) {
            return &filteredPlugins_[row];
        }
        return nullptr;
    }

    void filterPlugins(const juce::String& searchText) {
        filteredPlugins_.clear();

        // Filter based on searchText
        for (const auto& plugin : plugins_) {
            juce::String pluginName(plugin.name);
            if (pluginName.containsIgnoreCase(searchText)) {
                filteredPlugins_.push_back(plugin);
            }
        }
    }

    void resetFilters() {
        filteredPlugins_ = plugins_;
    }

private:
    std::shared_ptr<IPluginManager> pluginManager_;
    std::shared_ptr<IActionHandler> actionHandler_;
    std::shared_ptr<WindowManager> windowManager_;
    std::shared_ptr<Theme> theme_;
    const std::vector<Plugin>& plugins_;
    std::vector<Plugin> filteredPlugins_;
    const Plugin* selectedPlugin_ = nullptr;
};

SearchBox::SearchBox(
                     std::function<std::shared_ptr<IPluginManager>()> pluginManager
                     , std::function<std::shared_ptr<EventHandler>()> eventHandler
                     , std::function<std::shared_ptr<IActionHandler>()> actionHandler
                     , std::function<std::shared_ptr<WindowManager>()> windowManager
                     , std::function<std::shared_ptr<Theme>()> theme
                     , std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel
    )
    : TopLevelWindow("SearchBox", true)
    , pluginManager_(std::move(pluginManager))
    , eventHandler_(std::move(eventHandler))
    , actionHandler_(std::move(actionHandler))
    , windowManager_(std::move(windowManager))
    , theme_(std::move(theme))
    , limLookAndFeel_(std::move(limLookAndFeel))
    {

    logger->debug("Creating SearchBoxWindowController");

    setOpaque(false);
    setDropShadowEnabled(false);

    // Remove any default border
    setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);

    juce::LookAndFeel::setDefaultLookAndFeel(limLookAndFeel_().get());

    pluginListModel_ = std::make_unique<PluginListModel>(pluginManager_(), actionHandler_(), windowManager_(), theme_());
    listBox_.setModel(pluginListModel_.get());
    addAndMakeVisible(listBox_);

    setUsingNativeTitleBar(false);
    setAlwaysOnTop(true);

    searchField_.setIndents(searchField_.getLeftIndent(), 0);
    searchField_.setJustification(juce::Justification::centredLeft);
    searchField_.setMultiLine(false);
    searchField_.setReturnKeyStartsNewLine(false);
    searchField_.setTextToShowWhenEmpty("Search", theme_()->getColorValue("RetroDisplayForegroundDisabled"));
    searchField_.addListener(this);
    searchField_.addKeyListener(this);
    addAndMakeVisible(searchField_);

    setWindowGeometry();
}

SearchBox::~SearchBox() {}

void SearchBox::textEditorTextChanged(juce::TextEditor& editor) {
    if (&editor == &searchField_) {
        logger->debug("Search text changed: " + editor.getText().toStdString());
        juce::String searchText = searchField_.getText();

        pluginListModel_->filterPlugins(searchText);

        listBox_.selectRow(0);

        listBox_.updateContent();
        listBox_.repaint();
    }
}

bool SearchBox::keyPressed(const juce::KeyPress& key, juce::Component*) {
    logger->info("search box key pressed: " + std::to_string(key.getKeyCode()));

    if (key == juce::KeyPress::escapeKey) {
        if (searchField_.getText().isNotEmpty()) {
            searchField_.clear();
            resetFilters();
        } else {
            windowManager_()->closeWindow("SearchBox");
        }
        return true;  // Key press handled
    }

    if (key == juce::KeyPress::returnKey) {
        logger->info("enter key pressed");
        int selectedRow = listBox_.getSelectedRow();
        const Plugin* plugin = pluginListModel_->getPluginAtRow(selectedRow);
        if (plugin) {
            int pluginID = plugin->number;
            juce::Timer::callAfterDelay(100, [this, pluginID]() {
                actionHandler_()->loadItem(pluginID);
                windowManager_()->closeWindow("SearchBox");
            });
        }
        return true;
    }

    if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey ||
        key == juce::KeyPress::pageUpKey || key == juce::KeyPress::pageDownKey)
    {
        int currentIndex = listBox_.getSelectedRow();
        int numRows = listBox_.getListBoxModel()->getNumRows();
        if (!numRows) {
            return true;
        }

        int visibleRows = listBox_.getHeight() / listBox_.getRowHeight();

        if (key == juce::KeyPress::upKey && currentIndex > 0) {
            listBox_.selectRow(currentIndex - 1);
        } else if (key == juce::KeyPress::downKey && currentIndex < numRows - 1) {
            listBox_.selectRow(currentIndex + 1);
        } else if (key == juce::KeyPress::pageUpKey && currentIndex > 0) {
            int targetRow = juce::jmax(0, currentIndex - visibleRows);
            listBox_.selectRow(targetRow);
        } else if (key == juce::KeyPress::pageDownKey && currentIndex < numRows - 1) {
            int targetRow = juce::jmin(numRows - 1, currentIndex + visibleRows);
            listBox_.selectRow(targetRow);
        }

        // Keep focus on the searchField_
        searchField_.grabKeyboardFocus();
        return true;  // Key press handled
    }


    return false;  // Key press not handled
}

void SearchBox::setSelectedRow(int row) {
    selectedRow_ = row;
}

void SearchBox::resetFilters() {
    pluginListModel_->resetFilters();
    listBox_.updateContent();
    listBox_.repaint();
}

void SearchBox::setWindowGeometry() {
    // TODO: store conf if the window has been moved
    // or resized then override these with values
    // from conf
    //
    // TODO: works with dual monitors?
    // works with two ableton windows open?
    // Get the primary screen's dimensions
    auto* primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

    if (primaryDisplay != nullptr)
    {
        int screenWidth = primaryDisplay->totalArea.getWidth();
        int screenHeight = primaryDisplay->totalArea.getHeight();

        // Get Ableton Live's window bounds (assuming you have this method)
        ERect liveBounds = eventHandler_()->getLiveBoundsRect();
        int widgetWidth = 350;

        // TODO calculate height from listbox row height + searchBox + padding
        int widgetHeight = 300;

        // Center the widget inside Ableton Live's bounds
        int xPos = liveBounds.x + (liveBounds.width - widgetWidth) / 2;
        int yPos = liveBounds.y + (liveBounds.height - widgetHeight) / 2;

        // Ensure the window is within the screen boundaries
        xPos = std::max(0, std::min(xPos, screenWidth - widgetWidth));
        yPos = std::max(0, std::min(yPos, screenHeight - widgetHeight));

        // Set the position and size of the window
        setBounds(xPos, yPos, widgetWidth, widgetHeight);
    }
}

void SearchBox::resized() {
    const int padding = 15;
    const int listBoxOffset = 10;

    auto bounds = getLocalBounds().reduced(padding);
    searchField_.setBounds(bounds.removeFromTop(30));

    listBox_.setRowHeight(20);
    int numVisibleItems = 11;
    int totalHeight = listBox_.getRowHeight() * numVisibleItems;

    listBox_.setBounds(bounds.translated(0, listBoxOffset));
    listBox_.setBounds(listBox_.getX(), listBox_.getY(), listBox_.getWidth(), totalHeight);

//    juce::Rectangle<int> listBoxBounds = getLocalBounds();
//    listBoxBounds = listBoxBounds.withTrimmedBottom(5);
//    listBox_.setBounds(listBoxBounds);
}

void SearchBox::open() {
    setWindowGeometry();

    eventHandler_()->focusLim();
    eventHandler_()->focusWindow(this->getWindowHandle());

    listBox_.selectRow(0);
    setVisible(true);
    searchField_.setWantsKeyboardFocus(true);
    searchField_.grabKeyboardFocus();
    toFront(true);

    // hack for macOS
    // https://forum.juce.com/t/keyboard-focus-on-application-startup/9382/2
    juce::Timer::callAfterDelay(100, [this]() {
        focus();
    });
}

void SearchBox::focus() {
    // hack for macOS
    // https://forum.juce.com/t/keyboard-focus-on-application-startup/9382/2
    if (!searchField_.hasKeyboardFocus(false)) {
        searchField_.grabKeyboardFocus();
        juce::Timer::callAfterDelay(100, [this]() { focus(); });
    }
}

void SearchBox::mouseDown(const juce::MouseEvent& event) {
    searchField_.grabKeyboardFocus();

    juce::Component::mouseDown(event);
}

void SearchBox::close() {
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        setVisible(false);
        searchField_.clear();
        resetFilters();
    } else {
        juce::MessageManager::callAsync([this]() {
            setVisible(false);
            searchField_.clear();
            resetFilters();
        });
    }
}

void* SearchBox::getWindowHandle() const {
    void* handle = nullptr;

    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        handle = (void*)Component::getWindowHandle();
    } else {
        juce::WaitableEvent event;

        juce::MessageManager::callAsync([this, &handle, &event]() {
            handle = (void*)Component::getWindowHandle();
            logger->debug("SearchBox window handle: " + std::to_string(reinterpret_cast<uintptr_t>(handle)));
            event.signal();
        });

        event.wait();
    }

    return handle;
}

int SearchBox::getNumRows() {
    return static_cast<int>(filteredOptions_.size());
}

void SearchBox::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(0.5f); // Reduce by 0.5 to avoid anti-aliasing artifacts

    g.setColour(theme_()->getColorValue("SceneContrast"));
    g.fillRoundedRectangle(bounds, 6.0f); // bounds, cornerSize

    // Draw the border
    g.setColour(theme_()->getColorValue("SelectionFrame"));
    g.drawRoundedRectangle(bounds.reduced(8.0f), 6.0f, 2.0f); // bounds, cornerSize, borderThickness
                                                 //
    // Fill the background
    g.setColour(theme_()->getColorValue("SurfaceBackground"));
    g.fillRoundedRectangle(bounds.reduced(10.0f), 3.0f); // bounds, cornerSize

//    g.setColour(theme_()->getColorValue("SceneContrast"));
//    g.fillRoundedRectangle(bounds.reduced(11.0f), 3.0f); // bounds, cornerSize

    // Draw the text
    g.setFont(getHeight() * 0.7f);

}

void SearchBox::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) {
    return;
}
