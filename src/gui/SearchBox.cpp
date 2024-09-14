#include <JuceHeader.h>
#include <algorithm>

#include "LogHandler.h"
#include "ApplicationManager.h"
#include "SearchBox.h"
#include "Types.h"
#include "PID.h"

class PluginListModel : public juce::ListBoxModel {
public:
    PluginListModel()
        : plugins_(ApplicationManager::getInstance().getPlugins()) {}

    int getNumRows() override {
        return static_cast<int>(plugins_.size());
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override {
        if (rowIsSelected)
            g.fillAll(juce::Colours::lightblue);

        g.setColour(juce::Colours::black);
        if (rowNumber < plugins_.size()) {
            auto& plugin = plugins_[rowNumber];
            g.drawText(plugin.name, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override {
        if (row >= 0 && row < plugins_.size()) {
            selectedPlugin_ = &plugins_[row];
            juce::Logger::writeToLog("Selected plugin: " + selectedPlugin_->name);
        }
    }

    const Plugin* getSelectedPlugin() const {
        return selectedPlugin_;
    }

private:
    const std::vector<Plugin>& plugins_;
    const Plugin* selectedPlugin_ = nullptr;
};

SearchBox::SearchBox()
    : TopLevelWindow("SearchBox", true)
    , app_(ApplicationManager::getInstance()) {

    LogHandler::getInstance().debug("Creating SearchBoxWindowController");

    pluginListModel_ = std::make_unique<PluginListModel>();
    listBox_.setModel(pluginListModel_.get());
    listBox_.setRowHeight(30);
    addAndMakeVisible(listBox_);


    setUsingNativeTitleBar(false);
    setAlwaysOnTop(true);

    searchField_.setMultiLine(false);
    searchField_.setReturnKeyStartsNewLine(false);
//    searchField_.setTextToShowWhenEmpty("Type to search...", juce::Colours::grey);
    searchField_.addListener(this);
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
    }
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
        ERect liveBounds = app_.getEventHandler()->getLiveBoundsRect();
        int widgetWidth = 600;  // Width of the widget
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

void SearchBox::open() {
    listBox_.selectRow(0);
    setVisible(true);
    searchField_.grabKeyboardFocus();
    toFront(true);
}

void SearchBox::close() {
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        setVisible(false);
        //window->closeButtonPressed();
        searchField_.clear();
    } else {
        juce::MessageManager::callAsync([this]() {
            setVisible(false);
            //window->closeButtonPressed();
            searchField_.clear();
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


//std::string SearchBox::getSearchText() const {
//    if (searchField_) {
//        return searchField_->text().toStdString();
//    } else {
//        LogHandler::getInstance().error("searchField_ is not initialized");
//        return "";
//    }
//}

//size_t SearchBox::getSearchTextLength() const {
//    if (searchField_) {
//        return searchField_->text().length();
//    } else {
//        LogHandler::getInstance().error("searchField_ is not initialized");
//        return 0;
//    }
//}

//void SearchBox::setOptions(const std::vector<Plugin>& options) {
//    // Update the options and filteredOptions_ with the provided list of plugins
//    options_ = options;
//    filteredOptions_ = options_;  // Initially, all options are visible
//
//    // Tell the ListBox to refresh and display the new content
//    listBox_.updateContent();
//
//    LogHandler::getInstance().info("Options set successfully with " + std::to_string(options.size()) + " items.");
//}

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

void SearchBox::handlePluginSelected(int selectedRow) {
    if (selectedRow >= 0 && selectedRow < filteredOptions_.size()) {
        int index = filteredOptions_[selectedRow].number;
        LogHandler::getInstance().debug("Plugin selected: " + std::to_string(index));
        ApplicationManager::getInstance().getActionHandler()->loadItem(index);
        ApplicationManager::getInstance().getWindowManager()->closeWindow("SearchBox");
    }
}
int SearchBox::getNumRows() {
    return static_cast<int>(filteredOptions_.size());
}

void SearchBox::paint(juce::Graphics& g) {
    // Clear the background
    g.fillAll(juce::Colours::white);

    // Set the color for drawing
    g.setColour(juce::Colours::black);

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

void SearchBox::listBoxItemClicked(int row, const juce::MouseEvent&) {
    handlePluginSelected(row);
}

void SearchBox::resized() {
    // Set the bounds for search field and options list
    auto bounds = getLocalBounds();
    searchField_.setBounds(bounds.removeFromTop(30));
    listBox_.setBounds(bounds);
}

//void SearchBox::mousePressEvent(QMouseEvent* event) {
//    if (event->button() == Qt::LeftButton) {
//        mousePressed_ = true;
//        mouseStartPosition_ = event->globalPosition().toPoint();
//        windowStartPosition_ = this->frameGeometry().topLeft();
//    }
//}

//void SearchBox::mouseMoveEvent(QMouseEvent* event) {
//    if (mousePressed_) {
//        QPoint delta = event->globalPosition().toPoint() - mouseStartPosition_;
//        this->move(windowStartPosition_ + delta);
//    }
//}
//
//void SearchBox::mouseReleaseEvent(QMouseEvent* event) {
//    if (event->button() == Qt::LeftButton) {
//        mousePressed_ = false;
//    }
//}

//void SearchBox::onItemDoubleClicked(QListWidgetItem* item) {
//    if (!item) {
//        LogHandler::getInstance().error("No item selected on double-click");
//        return;
//    }
//
//    handlePluginSelected(item);
//}

//void SearchBox::keyPressEvent(QKeyEvent* event) {
//    QWidget::keyPressEvent(event);
//
//    if (event->modifiers() & Qt::ShiftModifier) {
//        LogHandler::getInstance().info("Shift modifier is pressed");
//    }
//    if (event->modifiers() & Qt::ControlModifier) {
//        LogHandler::getInstance().info("Control modifier is pressed");
//    }
//    if (event->modifiers() & Qt::AltModifier) {
//        LogHandler::getInstance().info("Alt modifier is pressed");
//    }
//    if (event->modifiers() & Qt::MetaModifier) {
//        LogHandler::getInstance().info("Meta (Command) modifier is pressed");
//    }
//
//    int itemsPerPage = optionsList_->height() / optionsList_->sizeHintForRow(0);
//
//    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
//        LogHandler::getInstance().info("cmd+f pressed");
//        searchField_->setFocus();
//        searchField_->selectAll();
//
//    } else if (event->key() == Qt::Key_Up) {
//        LogHandler::getInstance().info("Up pressed");
//
//        int currentIndex = optionsList_->currentRow();
//        if (currentIndex > 0) {
//            for (int i = currentIndex - 1; i >= 0; --i) {
//                if (!optionsList_->item(i)->isHidden()) {
//                    optionsList_->setCurrentRow(i);
//                    break;
//                }
//            }
//        }
//
//    } else if (event->key() == Qt::Key_Down) {
//        LogHandler::getInstance().info("Down pressed");
//
//        int currentIndex = optionsList_->currentRow();
//        if (currentIndex < optionsList_->count() - 1) {
//            for (int i = currentIndex + 1; i < optionsList_->count(); ++i) {
//                if (!optionsList_->item(i)->isHidden()) {
//                    optionsList_->setCurrentRow(i);
//                    break;
//                }
//            }
//        }
//
//    } else if (event->key() == Qt::Key_PageUp) {
//        LogHandler::getInstance().info("PageUp pressed");
//
//        int currentIndex = optionsList_->currentRow();
//        int itemsMoved = 0;
//
//        // Move up by `itemsPerPage` visible items
//        for (int i = currentIndex - 1; i >= 0 && itemsMoved < itemsPerPage; --i) {
//            if (!optionsList_->item(i)->isHidden()) {
//                itemsMoved++;
//                if (itemsMoved == itemsPerPage) {
//                    optionsList_->setCurrentRow(i);
//                }
//            }
//        }
//
//        // If less than `itemsPerPage` items were moved, select the first visible item
//        if (itemsMoved < itemsPerPage) {
//            for (int i = 0; i <= currentIndex; ++i) {
//                if (!optionsList_->item(i)->isHidden()) {
//                    optionsList_->setCurrentRow(i);
//                    break;
//                }
//            }
//        }
//
//    } else if (event->key() == Qt::Key_PageDown) {
//        LogHandler::getInstance().info("PageDown pressed");
//
//        int currentIndex = optionsList_->currentRow();
//        int itemsMoved = 0;
//
//        // Move down by `itemsPerPage` visible items
//        for (int i = currentIndex + 1; i < optionsList_->count() && itemsMoved < itemsPerPage; ++i) {
//            if (!optionsList_->item(i)->isHidden()) {
//                itemsMoved++;
//                if (itemsMoved == itemsPerPage) {
//                    optionsList_->setCurrentRow(i);
//                }
//            }
//        }
//
//        // If less than `itemsPerPage` items were moved, select the last visible item
//        if (itemsMoved < itemsPerPage) {
//            for (int i = optionsList_->count() - 1; i >= currentIndex; --i) {
//                if (!optionsList_->item(i)->isHidden()) {
//                    optionsList_->setCurrentRow(i);
//                    break;
//                }
//            }
//        }
//
//    } else if (event->key() == Qt::Key_Home) {
//        LogHandler::getInstance().info("Home pressed");
//        for (int i = 0; i < optionsList_->count(); ++i) {
//            if (!optionsList_->item(i)->isHidden()) {
//                optionsList_->setCurrentRow(i);
//                break;
//            }
//        }
//
//    } else if (event->key() == Qt::Key_End) {
//        LogHandler::getInstance().info("End pressed");
//        for (int i = optionsList_->count() - 1; i >= 0; --i) {
//            if (!optionsList_->item(i)->isHidden()) {
//                optionsList_->setCurrentRow(i);
//                break;
//            }
//        }
//
//    } else if (event->key() == Qt::Key_Escape) {
//        if (searchField_->text().length()) {
//            searchField_->clear();
//            optionsList_->clearSelection();
//        } else {
//            closeSearchBox();
//        }
//
//    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
//        LogHandler::getInstance().info("enter pressed");
//        QListWidgetItem* selectedItem = optionsList_->currentItem();
//
//        if (selectedItem && selectedItem->isSelected()) {
//            handlePluginSelected(selectedItem);
//        }
//
//    } else {
//        LogHandler::getInstance().info("KEY pressed: " + std::to_string(event->key()));
//    }
//
//
//    // Log the key press event if needed
//    LogHandler::getInstance().info("Key Pressed: " + std::to_string(event->key()));
//}
//
//// Handle key release events
//void SearchBox::keyReleaseEvent(QKeyEvent* event) {
//    QWidget::keyReleaseEvent(event);  // Call the base class implementation
//
//    // Log the key release event if needed
//    LogHandler::getInstance().info("Key Released: " + std::to_string(event->key()));
//}
