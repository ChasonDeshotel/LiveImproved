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
    , isOpen_(false)
    , searchField_(new QLineEdit())
    , qtWidget_(new FocusedWidget(searchField_))
    , optionsList_(new QListWidget())
{

    LogHandler::getInstance().info("Creating GUISearchBoxWindowController");

    connect(qtWidget_, &FocusedWidget::requestClose, this, &GUISearchBox::closeSearchBox);

    qtWidget_->setGeometry(100, 100, 400, 300);
    qtWidget_->resize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(qtWidget_);

    searchField_->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(searchField_);
    layout->addWidget(optionsList_);

    qtWidget_->setLayout(layout);

    QObject::connect(searchField_, &QLineEdit::textChanged, [this](const QString &text) {
        this->filterOptions(text);
    });
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
        item->setData(Qt::UserRole, QVariant::fromValue(&plugin)); // Store the plugin pointer if needed
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

        // Split the item text into words
        QStringList words = itemText.split(' ', Qt::SkipEmptyParts);

        // Check if any word starts with the input text
        bool match = false;
        for (const QString& word : words) {
            if (word.startsWith(text, Qt::CaseInsensitive)) {
                match = true;
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


void GUISearchBox::handlePluginSelected(const Plugin& plugin) {
    // Handle the selected Plugin object on the C++ side
    LogHandler::getInstance().info("Plugin selected: " + plugin.name);
    // Perform additional actions as needed
}
