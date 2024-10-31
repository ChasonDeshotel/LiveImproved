#define NOMINMAX
#include <JuceHeader.h>
#include <algorithm>
#include <functional>

#include "LogGlobal.h"
#include "Types.h"

#include "IActionHandler.h"
#include "IEventHandler.h"
#include "IWindowManager.h"

#include "LimLookAndFeel.h"
#include "PluginManager.h"
#include "SearchBox.h"
#include "Theme.h"

class PluginListModel : public juce::ListBoxModel {
public:
    PluginListModel(std::shared_ptr<IPluginManager> pluginManager, std::shared_ptr<IActionHandler> actionHandler, std::shared_ptr<IWindowManager> windowManager, std::shared_ptr<Theme> theme, int delayBeforeClose)
        : pluginManager_(std::move(pluginManager))
        , actionHandler_(std::move(actionHandler))
        , windowManager_(std::move(windowManager))
        , theme_(std::move(theme))
        , plugins_(pluginManager_->getPlugins())
        , filteredPlugins_(plugins_)
        , delayBeforeClose_(delayBeforeClose)
    {}

    auto getNumRows() -> int override {
        return static_cast<int>(filteredPlugins_.size());
    }

    auto paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) -> void override {
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

    auto listBoxItemClicked(int row, const juce::MouseEvent&) -> void override {
        if (row >= 0 && row < filteredPlugins_.size()) {
            const Plugin* plugin = getPluginAtRow(row);
            if (plugin) {
                int pluginID = plugin->number;
                juce::Timer::callAfterDelay(delayBeforeClose_, [this, pluginID]() {
                    actionHandler_->loadItem(pluginID);
                    windowManager_->closeWindow("SearchBox");
                });
            }
        }
    }

    [[nodiscard]] auto getPluginAtRow(int row) const -> const Plugin* {
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

public:
    int delayBeforeClose_;
private:
    std::shared_ptr<IPluginManager> pluginManager_;
    std::shared_ptr<IActionHandler> actionHandler_;
    std::shared_ptr<IWindowManager> windowManager_;
    std::shared_ptr<Theme> theme_;
    const std::vector<Plugin>& plugins_;
    std::vector<Plugin> filteredPlugins_;
    const Plugin* selectedPlugin_ = nullptr;
};

SearchBox::SearchBox(
                     std::function<std::shared_ptr<IPluginManager>()> pluginManager
                     , std::function<std::shared_ptr<IEventHandler>()> eventHandler
                     , std::function<std::shared_ptr<IActionHandler>()> actionHandler
                     , std::function<std::shared_ptr<IWindowManager>()> windowManager
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
    , selectedRow_()
    {

    logger->debug("Creating SearchBoxWindowController");

    setOpaque(false);
    setDropShadowEnabled(false);

    // Remove any default border
    setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);

    juce::LookAndFeel::setDefaultLookAndFeel(limLookAndFeel_().get());

    pluginListModel_ = std::make_unique<PluginListModel>(pluginManager_(), actionHandler_(), windowManager_(), theme_(), DELAY_BEFORE_CLOSE);
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

SearchBox::~SearchBox() = default;

auto SearchBox::textEditorTextChanged(juce::TextEditor& editor) -> void {
    if (&editor == &searchField_) {
        logger->debug("Search text changed: " + editor.getText().toStdString());
        juce::String searchText = searchField_.getText();

        pluginListModel_->filterPlugins(searchText);

        listBox_.selectRow(0);

        listBox_.updateContent();
        listBox_.repaint();
    }
}

auto SearchBox::keyPressed(const juce::KeyPress& key, juce::Component*) -> bool {
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
            juce::Timer::callAfterDelay(DELAY_BEFORE_CLOSE, [this, pluginID]() {
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

auto SearchBox::setSelectedRow(int row) -> void {
    selectedRow_ = row;
}

auto SearchBox::resetFilters() -> void {
    pluginListModel_->resetFilters();
    listBox_.updateContent();
    listBox_.repaint();
}

auto SearchBox::setWindowGeometry() -> void {
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

        // Center the widget inside Ableton Live's bounds
        int xPos = liveBounds.x + (liveBounds.width - WIDGET_WIDTH) / 2;
        int yPos = liveBounds.y + (liveBounds.height - WIDGET_HEIGHT) / 2;

        // Ensure the window is within the screen boundaries
        xPos = std::max(0, std::min(xPos, screenWidth - WIDGET_WIDTH));
        yPos = std::max(0, std::min(yPos, screenHeight - WIDGET_HEIGHT));

        // Set the position and size of the window
        setBounds(xPos, yPos, WIDGET_WIDTH, WIDGET_HEIGHT);
    }
}

auto SearchBox::resized() -> void {
    const int listBoxOffset = 10;

    auto bounds = getLocalBounds().reduced(PADDING);
    searchField_.setBounds(bounds.removeFromTop(SEARCHBOX_REMOVE_FROM_TOP));

    listBox_.setRowHeight(ROW_HEIGHT);
    int totalHeight = listBox_.getRowHeight() * NUM_VISIBLE_ITEMS;

    listBox_.setBounds(bounds.translated(0, listBoxOffset));
    listBox_.setBounds(listBox_.getX(), listBox_.getY(), listBox_.getWidth(), totalHeight);

//    juce::Rectangle<int> listBoxBounds = getLocalBounds();
//    listBoxBounds = listBoxBounds.withTrimmedBottom(5);
//    listBox_.setBounds(listBoxBounds);
}

auto SearchBox::open() -> void {
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
    juce::Timer::callAfterDelay(DELAY_BEFORE_FOCUS, [this]() {
        focus();
    });
}

auto SearchBox::focus() -> void {
    // hack for macOS
    // https://forum.juce.com/t/keyboard-focus-on-application-startup/9382/2
    if (!searchField_.hasKeyboardFocus(false)) {
        searchField_.grabKeyboardFocus();
        juce::Timer::callAfterDelay(DELAY_BEFORE_FOCUS, [this]() { focus(); });
    }
}

auto SearchBox::mouseDown(const juce::MouseEvent& event) -> void {
    searchField_.grabKeyboardFocus();

    juce::Component::mouseDown(event);
}

auto SearchBox::close() -> void {
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

auto SearchBox::getWindowHandle() const -> void* {
    void* handle = nullptr;

    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        handle = (void*)Component::getWindowHandle();
    } else {
        juce::WaitableEvent event;

        juce::MessageManager::callAsync([this, &handle, &event]() {
            handle = (void*)Component::getWindowHandle();
            logger->debug("SearchBox window handle: " + std::to_string(reinterpret_cast<uintptr_t>(handle))); // NOLINT
            event.signal();
        });

        event.wait();
    }

    return handle;
}

auto SearchBox::getNumRows() -> int {
    return static_cast<int>(filteredOptions_.size());
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
auto SearchBox::paint(juce::Graphics& g) -> void {
    auto bounds = getLocalBounds().toFloat().reduced(0.5f); // NOLINT Reduce by 0.5 to avoid anti-aliasing artifacts

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
#ifdef __clang__
#pragma clang diagnostic pop
#endif

auto SearchBox::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) -> void {
    return;
}
