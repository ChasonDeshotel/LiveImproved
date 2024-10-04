#define NOMINMAX
#include <JuceHeader.h>
#include <algorithm>
#include <functional>

#include "LogHandler.h"
#include "Types.h"

#include "EventHandler.h"
#include "IActionHandler.h"
#include "PID.h"
#include "PluginManager.h"
#include "SearchBox.h"
#include "WindowManager.h"
#include "JuceTheme.h"
#include "Theme.h"

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
        if (rowIsSelected) {
            g.fillAll(theme_->getColorValue("SelectionBackground"));
            g.setColour(theme_->getColorValue("SelectionForeground"));
        } else {
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
                filteredPlugins_.push_back(plugin);  // Add matching plugins
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
                     std::function<std::shared_ptr<ILogHandler>()> logHandler
                     , std::function<std::shared_ptr<IPluginManager>()> pluginManager
                     , std::function<std::shared_ptr<EventHandler>()> eventHandler
                     , std::function<std::shared_ptr<IActionHandler>()> actionHandler
                     , std::function<std::shared_ptr<WindowManager>()> windowManager
                     , std::function<std::shared_ptr<Theme>()> theme
                     , std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel
    )
    : TopLevelWindow("SearchBox", true)
    , logHandler_(std::move(logHandler))
    , pluginManager_(std::move(pluginManager))
    , eventHandler_(std::move(eventHandler))
    , actionHandler_(std::move(actionHandler))
    , windowManager_(std::move(windowManager))
    , theme_(std::move(theme))
    , limLookAndFeel_(std::move(limLookAndFeel))
    {

    LogHandler::getInstance().debug("Creating SearchBoxWindowController");

    juce::LookAndFeel::setDefaultLookAndFeel(limLookAndFeel_().get());

    pluginListModel_ = std::make_unique<PluginListModel>(pluginManager_(), actionHandler_(), windowManager_(), theme_());
    listBox_.setModel(pluginListModel_.get());
    listBox_.setRowHeight(30);
    listBox_.setLookAndFeel(limLookAndFeel_().get());
    addAndMakeVisible(listBox_);

    setUsingNativeTitleBar(false);
    setAlwaysOnTop(true);

    searchField_.setMultiLine(false);
    searchField_.setReturnKeyStartsNewLine(false);
//    searchField_.setTextToShowWhenEmpty("Type to search...", juce::Colours::grey);
    searchField_.addListener(this);
    searchField_.addKeyListener(this);
    addAndMakeVisible(searchField_);

    // Configure the options list (ListBox in JUCE)
//    optionsList_.setModel(this);
//    addAndMakeVisible(optionsList_);

    setWindowGeometry();
}

SearchBox::~SearchBox() {}

void SearchBox::textEditorTextChanged(juce::TextEditor& editor) {
    if (&editor == &searchField_) {
        LogHandler::getInstance().debug("Search text changed: " + editor.getText().toStdString());
        // Handle the text change (e.g., update search results)
        juce::String searchText = searchField_.getText();

        // Call the filter method on the PluginListModel
        pluginListModel_->filterPlugins(searchText);

        listBox_.selectRow(0);

        listBox_.updateContent();
        listBox_.repaint();
    }
}

bool SearchBox::keyPressed(const juce::KeyPress& key, juce::Component*) {
    LogHandler::getInstance().info("search box key pressed: " + std::to_string(key.getKeyCode()));

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
        LogHandler::getInstance().info("enter key pressed");
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

    if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey) {
        int currentIndex = listBox_.getSelectedRow();
        int numRows = listBox_.getListBoxModel()->getNumRows();
        if (!numRows) {
            return true;
        }

        if (key == juce::KeyPress::upKey && currentIndex > 0) {
            listBox_.selectRow(currentIndex - 1);
        } else if (key == juce::KeyPress::downKey && currentIndex < numRows - 1) {
            listBox_.selectRow(currentIndex + 1);
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
        int widgetWidth = 350;  // Width of the widget
        int widgetHeight = 300; // Height of the widget

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

    listBox_.setBounds(bounds.translated(0, listBoxOffset));
}

void SearchBox::open() {
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
    if (!searchField_.hasKeyboardFocus(false)) {
        searchField_.grabKeyboardFocus();  // Try to grab focus
        juce::Timer::callAfterDelay(100, [this]() { focus(); });  // Continue checking until focus is gained
    }
}

void SearchBox::mouseDown(const juce::MouseEvent& event) {
    searchField_.grabKeyboardFocus();  // Force focus back to the search box

    // You can call the base class mouseDown if needed
    juce::Component::mouseDown(event);
}

void SearchBox::close() {
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        setVisible(false);
        //window->closeButtonPressed();
        searchField_.clear();
        resetFilters();
    } else {
        juce::MessageManager::callAsync([this]() {
            setVisible(false);
            //window->closeButtonPressed();
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
            LogHandler::getInstance().debug("SearchBox window handle: " + std::to_string(reinterpret_cast<uintptr_t>(handle)));
            event.signal();
        });

        event.wait();
    }

    return handle;
}

//void SearchBox::filterOptions(const QString &text) {
//    if (!optionsList_ || !originalItems_) {
//        LogHandler::getInstance().error("optionsList_ or originalItems_ is not initialized");
//        return;
//    }
//
//    optionsList_->clearSelection();
//    optionsList_->clear();
//
//    QListWidgetItem* firstVisibleItem = nullptr;
//
//    if (text.startsWith("r/")) {
//        if (text.length() == 2) {
//            QListWidgetItem* hintItem = new QListWidgetItem("Regex searching. Use r/<pattern>/");
//            hintItem->setFlags(hintItem->flags() & ~Qt::ItemIsSelectable);  // Make the item unselectable
//            optionsList_->addItem(hintItem);
//            return;
//        }
//
//        QString regexPattern = text.mid(2, text.length() - 3);
//        QRegularExpression regex(regexPattern, QRegularExpression::CaseInsensitiveOption);
//
//        if (!regex.isValid()) {
//            QListWidgetItem* errorItem = new QListWidgetItem("Invalid regex format. Use r/<pattern>/");
//            errorItem->setFlags(errorItem->flags() & ~Qt::ItemIsSelectable);
//            optionsList_->addItem(errorItem);
//            return;
//        }
//
//        for (int i = 0; i < originalItems_->count(); ++i) {
//            QListWidgetItem* item = originalItems_->item(i);
//            QString itemText = item->text();
//
//            QRegularExpressionMatch match = regex.match(itemText);
//
//            if (match.hasMatch()) {
//                optionsList_->addItem(new QListWidgetItem(*item));
//                if (!firstVisibleItem) {
//                    firstVisibleItem = optionsList_->item(optionsList_->count() - 1);
//                }
//            }
//        }
//    } else {
//        QString searchText = text.toLower();
//        QStringList searchWords = searchText.split(' ', Qt::SkipEmptyParts);
//
//        for (int i = 0; i < originalItems_->count(); ++i) {
//            QListWidgetItem* item = originalItems_->item(i);
//            QString itemText = item->text().toLower();
//
//            QStringList itemWords = itemText.split(' ', Qt::SkipEmptyParts);
//            QString acronym;
//            for (const QString& word : itemWords) {
//                if (!word.isEmpty()) {
//                    acronym += word[0];
//                }
//            }
//
//            bool match = false;
//
//            if (acronym.startsWith(searchText)) {
//                match = true;
//            } else {
//                for (const QString& searchWord : searchWords) {
//                    if (itemText.contains(searchWord)) {
//                        match = true;
//                        break;
//                    }
//                }
//            }
//
//            if (match) {
//                optionsList_->addItem(new QListWidgetItem(*item));
//                if (!firstVisibleItem) {
//                    firstVisibleItem = optionsList_->item(optionsList_->count() - 1);
//                }
//            }
//        }
//    }
//
//    if (firstVisibleItem) {
//        optionsList_->setCurrentItem(firstVisibleItem);
//        firstVisibleItem->setSelected(true);
//    }
//}

void SearchBox::handlePluginSelected() {
    int selectedRow = listBox_.getSelectedRow();
    const Plugin* plugin = pluginListModel_->getPluginAtRow(selectedRow);
    if (plugin) {
        actionHandler_()->loadItem(plugin->number);
        windowManager_()->closeWindow("SearchBox");
    }
}

int SearchBox::getNumRows() {
    return static_cast<int>(filteredOptions_.size());
}

void SearchBox::paint(juce::Graphics& g) {
    // Clear the background
    g.fillAll(theme_()->getColorValue("SurfaceBackground"));

    // Set the color for drawing
    g.setColour(theme_()->getColorValue("ControlForeground"));

    // Example: draw a rectangle
    g.drawRect(getLocalBounds(), 1);

    // Optionally, draw text or other graphics
    g.drawText("SearchBox", getLocalBounds(), juce::Justification::centred, true);
}

void SearchBox::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) {
    if (rowIsSelected) {
        g.fillAll(juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::TextEditor::highlightColourId));
    }

    g.setColour(juce::Colours::white);
    if (rowNumber < filteredOptions_.size()) {
        const auto& plugin = filteredOptions_[rowNumber];  // Get the plugin for this row
        g.drawText(plugin.name, 0, 0, width, height, juce::Justification::centredLeft);  // Draw plugin name
    }
}
