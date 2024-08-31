#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QKeyEvent>
#include <QListView>
#include <QString>

#include "LogHandler.h"
#include "ApplicationManager.h"
#include "SearchBox.h"
#include "Plugin.h"
#include "PID.h"

GUISearchBox::GUISearchBox(ApplicationManager& appManager)
    : title("foo")
    , app_(appManager)
    , isOpen_(false)
    , searchField_(new CustomLineEdit())
    , optionsList_(new QListWidget())
    , qtWidget_(this)
{

    LogHandler::getInstance().info("Creating GUISearchBoxWindowController");

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    ERect liveBounds = app_.getEventHandler()->getLiveBoundsRect();
    int widgetWidth = 600;  // Width of the widget
    int widgetHeight = 300; // Height of the widget
    int xPos = liveBounds.x + (liveBounds.width - widgetWidth) / 2;
    int yPos = liveBounds.y + (liveBounds.height - widgetHeight) / 2;

    this->setGeometry(xPos, yPos, widgetWidth, widgetHeight);

    optionsList_->setStyleSheet(
        "QListWidget::item:selected {"
        "background-color: palette(Highlight);"
        "color: palette(HighlightedText);"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(this);

    searchField_->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(searchField_);
    layout->addWidget(optionsList_);

    this->setLayout(layout);

    connect(searchField_, &QLineEdit::textChanged, [this](const QString &text) {
        this->filterOptions(text);
    });

    connect(optionsList_, &QListWidget::itemDoubleClicked, this, &GUISearchBox::onItemDoubleClicked);
}

GUISearchBox::~GUISearchBox() {}

QWidget* GUISearchBox::getQtWidget() const {
    return qtWidget_;
}

bool GUISearchBox::isOpen() const {
    return isOpen_;
}

void GUISearchBox::openSearchBox() {
    if (isOpen_) {
        LogHandler::getInstance().info("Search box is already open");
        return;
    }

    // Reset the current row to the first visible item
    for (int i = 0; i < optionsList_->count(); ++i) {
        if (!optionsList_->item(i)->isHidden()) {
            optionsList_->setCurrentRow(i);
            break;
        }
    }

    qtWidget_->show();
    searchField_->setFocus();
    isOpen_ = true;
    qtWidget_->raise();        // Raises the widget to the top of the stack
    qtWidget_->activateWindow();
    ApplicationManager::getInstance().getEventHandler()->focusApplication(PID::getInstance().appPID());
}

void GUISearchBox::closeEvent(QCloseEvent* event) {
    closeSearchBox();
    QWidget::closeEvent(event);
    ApplicationManager::getInstance().getEventHandler()->focusApplication(PID::getInstance().appPID());
}

void GUISearchBox::closeSearchBox() {
    if (!isOpen_) {
        LogHandler::getInstance().info("Search box is not open");
        return;
    }
    qtWidget_->hide();
    searchField_->clear();
    optionsList_->clearSelection();
    optionsList_->scrollToTop();

    // Additional optional reset: clear any hidden state if necessary
    for (int i = 0; i < optionsList_->count(); ++i) {
        optionsList_->item(i)->setHidden(false);  // Ensure all items are visible
    }

    isOpen_ = false;
    ApplicationManager::getInstance().getEventHandler()->focusApplication(PID::getInstance().livePID());
}

void GUISearchBox::clearSearchText() {
    searchField_->clear();
    optionsList_->clearSelection();
}

std::string GUISearchBox::getSearchText() const {
    if (searchField_) {
        return searchField_->text().toStdString();
    } else {
        LogHandler::getInstance().error("searchField_ is not initialized");
        return "";
    }
}

size_t GUISearchBox::getSearchTextLength() const {
    if (searchField_) {
        return searchField_->text().length();
    } else {
        LogHandler::getInstance().error("searchField_ is not initialized");
        return 0;
    }
}

void GUISearchBox::setOptions(const std::vector<Plugin>& options) {
    if (!optionsList_) {
        LogHandler::getInstance().error("optionsList_ is not initialized");
        return;
    }

    optionsList_->clear();

    // Iterate through the options and add them to the QListWidget
    for (const auto& plugin : options) {
        QString pluginName = QString::fromStdString(plugin.name);
        QListWidgetItem* item = new QListWidgetItem(pluginName);
        item->setData(Qt::UserRole, static_cast<int>(plugin.number));
        //item->setData(Qt::UserRole, QVariant::fromValue(&plugin)); // Store the plugin pointer if needed
        optionsList_->addItem(item);
    }

    LogHandler::getInstance().info("Options set successfully with " + std::to_string(options.size()) + " items.");
}

void GUISearchBox::filterOptions(const QString &text) {
    if (!optionsList_) {
        LogHandler::getInstance().error("optionsList_ is not initialized");
        return;
    }

    optionsList_->clearSelection();

    int visibleItemCount = 0;
    QListWidgetItem* lastVisibleItem = nullptr;

    for (int i = 0; i < optionsList_->count(); ++i) {
        QListWidgetItem* item = optionsList_->item(i);
        QString itemText = item->text();

        // Debugging: log the item text
        LogHandler::getInstance().info("Item text: " + itemText.toStdString());

        // Split the item text into words
        QStringList words = itemText.split(' ', Qt::SkipEmptyParts);

        // Check if any word starts with the input text
        bool match = false;
        for (const QString& word : words) {
            LogHandler::getInstance().info("Checking word: " + word.toStdString());
            if (word.startsWith(text, Qt::CaseInsensitive)) {
                match = true;
                LogHandler::getInstance().info("Match found: " + word.toStdString());
                break;
            }
        }

        item->setHidden(!match);
        if (match) {
            visibleItemCount++;
            lastVisibleItem = item;
        }
    }

    // If only one item remains visible, select it
    if (visibleItemCount == 1 && lastVisibleItem) {
        optionsList_->setCurrentItem(lastVisibleItem);
    }

    LogHandler::getInstance().info("Filtered options based on text: " + text.toStdString());
}

void GUISearchBox::handlePluginSelected(QListWidgetItem* selectedItem) {
    // Handle the selected Plugin object on the C++ side
    int index = selectedItem->data(Qt::UserRole).toInt();
    LogHandler::getInstance().info("plugin selected: " + std::to_string(index));
    ApplicationManager::getInstance().getActionHandler()->loadItem(index);
    closeSearchBox();
}

void GUISearchBox::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed_ = true;
        mouseStartPosition_ = event->globalPosition().toPoint();
        windowStartPosition_ = this->frameGeometry().topLeft();
    }
}

void GUISearchBox::mouseMoveEvent(QMouseEvent* event) {
    if (mousePressed_) {
        QPoint delta = event->globalPosition().toPoint() - mouseStartPosition_;
        this->move(windowStartPosition_ + delta);
    }
}

void GUISearchBox::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed_ = false;
    }
}

void GUISearchBox::onItemDoubleClicked(QListWidgetItem* item) {
    if (!item) {
        LogHandler::getInstance().error("No item selected on double-click");
        return;
    }

    handlePluginSelected(item);
}

void GUISearchBox::keyPressEvent(QKeyEvent* event) {
    QWidget::keyPressEvent(event);

    int itemsPerPage = optionsList_->height() / optionsList_->sizeHintForRow(0);

    if (event->key() == Qt::Key_Up) {
        LogHandler::getInstance().info("Up pressed");

        int currentIndex = optionsList_->currentRow();
        if (currentIndex > 0) {
            for (int i = currentIndex - 1; i >= 0; --i) {
                if (!optionsList_->item(i)->isHidden()) {
                    optionsList_->setCurrentRow(i);
                    break;
                }
            }
        }

    } else if (event->key() == Qt::Key_Down) {
        LogHandler::getInstance().info("Down pressed");

        int currentIndex = optionsList_->currentRow();
        if (currentIndex < optionsList_->count() - 1) {
            for (int i = currentIndex + 1; i < optionsList_->count(); ++i) {
                if (!optionsList_->item(i)->isHidden()) {
                    optionsList_->setCurrentRow(i);
                    break;
                }
            }
        }

    } else if (event->key() == Qt::Key_PageUp) {
        LogHandler::getInstance().info("PageUp pressed");

        int currentIndex = optionsList_->currentRow();
        int itemsMoved = 0;

        // Move up by `itemsPerPage` visible items
        for (int i = currentIndex - 1; i >= 0 && itemsMoved < itemsPerPage; --i) {
            if (!optionsList_->item(i)->isHidden()) {
                itemsMoved++;
                if (itemsMoved == itemsPerPage) {
                    optionsList_->setCurrentRow(i);
                }
            }
        }

        // If less than `itemsPerPage` items were moved, select the first visible item
        if (itemsMoved < itemsPerPage) {
            for (int i = 0; i <= currentIndex; ++i) {
                if (!optionsList_->item(i)->isHidden()) {
                    optionsList_->setCurrentRow(i);
                    break;
                }
            }
        }

    } else if (event->key() == Qt::Key_PageDown) {
        LogHandler::getInstance().info("PageDown pressed");

        int currentIndex = optionsList_->currentRow();
        int itemsMoved = 0;

        // Move down by `itemsPerPage` visible items
        for (int i = currentIndex + 1; i < optionsList_->count() && itemsMoved < itemsPerPage; ++i) {
            if (!optionsList_->item(i)->isHidden()) {
                itemsMoved++;
                if (itemsMoved == itemsPerPage) {
                    optionsList_->setCurrentRow(i);
                }
            }
        }

        // If less than `itemsPerPage` items were moved, select the last visible item
        if (itemsMoved < itemsPerPage) {
            for (int i = optionsList_->count() - 1; i >= currentIndex; --i) {
                if (!optionsList_->item(i)->isHidden()) {
                    optionsList_->setCurrentRow(i);
                    break;
                }
            }
        }

    } else if (event->key() == Qt::Key_Home) {
        LogHandler::getInstance().info("Home pressed");
        for (int i = 0; i < optionsList_->count(); ++i) {
            if (!optionsList_->item(i)->isHidden()) {
                optionsList_->setCurrentRow(i);
                break;
            }
        }

    } else if (event->key() == Qt::Key_End) {
        LogHandler::getInstance().info("End pressed");
        for (int i = optionsList_->count() - 1; i >= 0; --i) {
            if (!optionsList_->item(i)->isHidden()) {
                optionsList_->setCurrentRow(i);
                break;
            }
        }



    } else if (event->key() == Qt::Key_Escape) {
        if (searchField_->text().length()) {
            searchField_->clear();
        } else {
            closeSearchBox();
        }

    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        LogHandler::getInstance().info("enter pressed");
        QListWidgetItem* selectedItem = optionsList_->currentItem();
        LogHandler::getInstance().info("got current item");
        if (selectedItem && selectedItem->isSelected()) {
            handlePluginSelected(selectedItem);
        }
    }

    // Log the key press event if needed
    LogHandler::getInstance().info("Key Pressed: " + std::to_string(event->key()));
}

// Handle key release events
void GUISearchBox::keyReleaseEvent(QKeyEvent* event) {
    QWidget::keyReleaseEvent(event);  // Call the base class implementation

    // Log the key release event if needed
    LogHandler::getInstance().info("Key Released: " + std::to_string(event->key()));
}
