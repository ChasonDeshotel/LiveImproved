#include <algorithm>

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QKeyEvent>
#include <QListView>
#include <QString>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

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
    , originalItems_(new QListWidget())
    , qtWidget_(this)
{

    LogHandler::getInstance().info("Creating GUISearchBoxWindowController");

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    setWindowGeometry();

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

void GUISearchBox::setWindowGeometry() {
    // TODO: store conf if the window has been moved
    // or resized then override these with values
    // from conf
    //
    // TODO: works with dual monitors?
    // works with two ableton windows open?
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenWidth = screen->geometry().width();
    int screenHeight = screen->geometry().height();

    ERect liveBounds = app_.getEventHandler()->getLiveBoundsRect();
    int widgetWidth = 600;  // Width of the widget
    int widgetHeight = 300; // Height of the widget
    int xPos = liveBounds.x + (liveBounds.width - widgetWidth) / 2;
    int yPos = liveBounds.y + (liveBounds.height - widgetHeight) / 2;

    xPos = std::max(0, std::min(xPos, screenWidth - widgetWidth));
    yPos = std::max(0, std::min(yPos, screenHeight - widgetHeight));

    this->setGeometry(xPos, yPos, widgetWidth, widgetHeight);
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

    setWindowGeometry();
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
    originalItems_->clear();

    // Iterate through the options and add them to the QListWidget
    for (const auto& plugin : options) {
        QString pluginName = QString::fromStdString(plugin.name);
        QListWidgetItem* item = new QListWidgetItem(pluginName);
        item->setData(Qt::UserRole, static_cast<int>(plugin.number));
        //item->setData(Qt::UserRole, QVariant::fromValue(&plugin)); // Store the plugin pointer if needed
        optionsList_->addItem(item);
        originalItems_->addItem(new QListWidgetItem(*item));
    }

    LogHandler::getInstance().info("Options set successfully with " + std::to_string(options.size()) + " items.");
}

void GUISearchBox::filterOptions(const QString &text) {
    if (!optionsList_ || !originalItems_) {
        LogHandler::getInstance().error("optionsList_ or originalItems_ is not initialized");
        return;
    }

    optionsList_->clearSelection();
    optionsList_->clear();

    QListWidgetItem* firstVisibleItem = nullptr;

    if (text.startsWith("r/")) {
        if (text.length() == 2) {
            QListWidgetItem* hintItem = new QListWidgetItem("Regex searching. Use r/<pattern>/");
            hintItem->setFlags(hintItem->flags() & ~Qt::ItemIsSelectable);  // Make the item unselectable
            optionsList_->addItem(hintItem);
            return;
        }

        QString regexPattern = text.mid(2, text.length() - 3);
        QRegularExpression regex(regexPattern, QRegularExpression::CaseInsensitiveOption);

        if (!regex.isValid()) {
            QListWidgetItem* errorItem = new QListWidgetItem("Invalid regex format. Use r/<pattern>/");
            errorItem->setFlags(errorItem->flags() & ~Qt::ItemIsSelectable);
            optionsList_->addItem(errorItem);
            return;
        }

        for (int i = 0; i < originalItems_->count(); ++i) {
            QListWidgetItem* item = originalItems_->item(i);
            QString itemText = item->text();

            QRegularExpressionMatch match = regex.match(itemText);

            if (match.hasMatch()) {
                optionsList_->addItem(new QListWidgetItem(*item));
                if (!firstVisibleItem) {
                    firstVisibleItem = optionsList_->item(optionsList_->count() - 1);
                }
            }
        }
    } else {
        QString searchText = text.toLower();
        QStringList searchWords = searchText.split(' ', Qt::SkipEmptyParts);

        for (int i = 0; i < originalItems_->count(); ++i) {
            QListWidgetItem* item = originalItems_->item(i);
            QString itemText = item->text().toLower();

            QStringList itemWords = itemText.split(' ', Qt::SkipEmptyParts);
            QString acronym;
            for (const QString& word : itemWords) {
                if (!word.isEmpty()) {
                    acronym += word[0];
                }
            }

            bool match = false;

            if (acronym.startsWith(searchText)) {
                match = true;
            } else {
                for (const QString& searchWord : searchWords) {
                    if (itemText.contains(searchWord)) {
                        match = true;
                        break;
                    }
                }
            }

            if (match) {
                optionsList_->addItem(new QListWidgetItem(*item));
                if (!firstVisibleItem) {
                    firstVisibleItem = optionsList_->item(optionsList_->count() - 1);
                }
            }
        }
    }

    if (firstVisibleItem) {
        optionsList_->setCurrentItem(firstVisibleItem);
        firstVisibleItem->setSelected(true);
    }
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

    if (event->modifiers() & Qt::ShiftModifier) {
        LogHandler::getInstance().info("Shift modifier is pressed");
    }
    if (event->modifiers() & Qt::ControlModifier) {
        LogHandler::getInstance().info("Control modifier is pressed");
    }
    if (event->modifiers() & Qt::AltModifier) {
        LogHandler::getInstance().info("Alt modifier is pressed");
    }
    if (event->modifiers() & Qt::MetaModifier) {
        LogHandler::getInstance().info("Meta (Command) modifier is pressed");
    }

    int itemsPerPage = optionsList_->height() / optionsList_->sizeHintForRow(0);

    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        LogHandler::getInstance().info("cmd+f pressed");
        searchField_->setFocus();
        searchField_->selectAll();

    } else if (event->key() == Qt::Key_Up) {
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
            optionsList_->clearSelection();
        } else {
            closeSearchBox();
        }

    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        LogHandler::getInstance().info("enter pressed");
        QListWidgetItem* selectedItem = optionsList_->currentItem();

        if (selectedItem && selectedItem->isSelected()) {
            handlePluginSelected(selectedItem);
        }

    } else {
        LogHandler::getInstance().info("KEY pressed: " + std::to_string(event->key()));
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
