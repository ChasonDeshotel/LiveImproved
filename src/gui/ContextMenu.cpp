#include <fstream>
#include <iostream>

#include <QApplication>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QWindow>
#include <QAction>
#include <QCursor>
#include <QWidget>

#include <iostream>
#include <string>
#include <vector>

#include <QMenu>
#include <QMenuBar>

#include <yaml-cpp/yaml.h>

#include "ApplicationManager.h"
#include "ContextMenu.h"
#include "LogHandler.h"
#include "PID.h"
#include "Types.h"

ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent)
    , isOpen_(false)
    //, activeMenu_(this)
{

    // Set the window flag to keep the menu on top
    setWindowFlag(Qt::WindowStaysOnTopHint);

    connect(this, &QMenu::aboutToHide, this, [this]() {
        LogHandler::getInstance().info("Context menu is closing");
        isOpen_ = false;
        //ApplicationManager::getInstance().getEventHandler()->focusLive();
    });

    //connectMenuSignals(this);

    QStringList menuItems = {
        "/Menu1", "Item1", "Item2", "--", "Item3", "--",
        "/Menu2", "Foo", "Bar", "//Submenu2", "Test", "///Submenu2Submenu", "AnotherItem"
    };

    buildMenu();
}

//void ContextMenu::connectMenuSignals(QMenu *menu) {
//    // Connect the aboutToShow signal to update activeMenu_
//    connect(menu, &QMenu::aboutToShow, this, [this, menu]() {
//        activeMenu_ = menu;  // Set this menu as the active menu when it's about to show
//    });
//
//    // Connect the hovered signal to update activeMenu_ when a submenu is hovered
//    connect(menu, &QMenu::hovered, this, [this](QAction *action) {
//        if (action && action->menu()) {
//            activeMenu_ = action->menu();  // Set submenu as the active menu when hovered
//        }
//    });
//}

bool ContextMenu::isOpen() const {
    return isOpen_;
}

//QMenu* ContextMenu::getActiveMenu() const {
//    return activeMenu_;  // Returns the current active menu (parent or submenu)
//}

void ContextMenu::keyPressEvent(QKeyEvent *event) {
    LogHandler::getInstance().info("key press event from menu");
    QMenu::keyPressEvent(event);
    return;
    QAction* currentAction = this->activeAction();

    switch (event->key()) {

        default:
            QMenu::keyPressEvent(event);
            break;
    }
   

}

//QAction* ContextMenu::findNextAction(QAction* currentAction, bool forward) {
//    const auto& actions = this->actions();
//    if (actions.isEmpty()) return nullptr;
//
//    int currentIndex = actions.indexOf(currentAction);
//    int nextIndex = (forward) ? (currentIndex + 1) % actions.size() : (currentIndex - 1 + actions.size()) % actions.size();
//    return actions.at(nextIndex);
//}

//QMenu* ContextMenu::createMenuFromYAML(const YAML::Node& node, QWidget* parent) {
//    QMenu* menu = new QMenu(node["name"].as<std::string>().c_str(), parent);
//
//    // Iterate through the items
//    for (const auto& item : node["items"]) {
//        std::string label = item["label"].as<std::string>();
//
//        if (label == "--") {
//            // Add a separator if the label is "--"
//            menu->addSeparator();
//        } else if (item["subcategory"]) {
//            // If the item has a subcategory, create a submenu recursively
//            QMenu* submenu = createMenuFromYAML(item["subcategory"], menu);
//            menu->addMenu(submenu);
//        } else {
//            // Otherwise, add an action
//            QAction* action = new QAction(label.c_str(), menu);
//            QObject::connect(action, &QAction::triggered, [item]() {
//                std::string actionCommand = item["action"].as<std::string>();
//                std::cout << "Action triggered: " << actionCommand << std::endl;
//            });
//            menu->addAction(action);
//        }
//    }
//
//    return menu;
//}

// Function to create QWindow from vector of MenuCategories
void ContextMenu::buildMenu(const std::vector<MenuCategory>& categories) {
//QWindow* createWindowFromMenuCategories(const std::vector<MenuCategory>& categories) {
    QWidget* mainWidget = new QWidget();  // Main container widget
    QVBoxLayout* layout = new QVBoxLayout();  // Vertical layout for categories

    // Iterate through each category and add buttons for each item
    for (const auto& category : categories) {
        QLabel* categoryLabel = new QLabel(QString::fromStdString(category.name));
        layout->addWidget(categoryLabel);  // Add category label

        for (const auto& item : category.items) {
            QPushButton* button = new QPushButton(QString::fromStdString(item.label));
            layout->addWidget(button);

            // Connect the button to an action
            QObject::connect(button, &QPushButton::clicked, [item]() {
                // Example action: Show a message box with the action
                QMessageBox::information(nullptr, "Action", QString::fromStdString(item.action));
            });
        }
    }

    mainWidget->setLayout(layout);  // Set layout for the widget
    mainWidget->show();  // Show the widget as a window

//    return mainWidget->windowHandle();  // Return the window handle
}

//int main(int argc, char* argv[]) {
//    QApplication app(argc, argv);
//
//    // Example menu categories
//    std::vector<MenuCategory> categories = {
//        {"Saturation", {{"Amplitube", "AmpliTube 5 VST3"}, {"Bias Amp", "BIAS AMP 2 VST"}}},
//        {"Mastering", {{"Elephant", "Elephant VST3"}, {"ChannelStrip", "ChannelStrip AU"}}}
//    };
//
//    QWindow* window = createWindowFromMenuCategories(categories);
//    window->show();
//
//    return app.exec();
//}



//std::vector<MenuCategory> menuData = ApplicationManager::getInstance().getConfigMenu()->getMenuData();
//    QMenu *currentMenu = this;  // Use QMenu* instead of ContextMenu*
//    for (const auto& category : menuData) {
//        currentMenu->addSeparator();
//        QString submenuName = QString::fromStdString(category.name);
//        QMenu *submenu = new QMenu(submenuName, this);
//
//        submenus[submenuName] = submenu;
//        currentMenu->addMenu(submenu);
//        currentMenu = submenu;
//    }
//}

//void ContextMenu::buildMenu(const QStringList &menuItems) {
//    QMenu *currentMenu = this;  // Use QMenu* instead of ContextMenu*
//
//    QMap<QString, QMenu*> submenus;
//
////    QLineEdit *searchBox = new QLineEdit(this);
////    searchBox->setPlaceholderText("Search...");
////
////    // Create a QWidgetAction to hold the QLineEdit
////    QWidgetAction *searchAction = new QWidgetAction(this);
////    searchAction->setDefaultWidget(searchBox);
////
////    this->addAction(searchAction);
////
////    connect(searchBox, &QLineEdit::textChanged, this, [this](const QString &text) {
////        // Implement search/filter logic here
////        qDebug() << "Searching for: " << text;
////    });
//
//
//    for (const QString &item : menuItems) {
//        if (item == "--") {
//            currentMenu->addSeparator();
//        } else if (item.startsWith("///")) {
//            QString submenuName = item.mid(3);
//            QMenu *submenu = new QMenu(submenuName, this);
//
//            //connectMenuSignals(this);
//
//            submenus[submenuName] = submenu;
//            currentMenu->addMenu(submenu);
//            currentMenu = submenu;
//        } else if (item.startsWith("//")) {
//            QString submenuName = item.mid(2);
//            QMenu *submenu = new QMenu(submenuName, this);
//
//            //connectMenuSignals(this);
//
//            submenus[submenuName] = submenu;
//            currentMenu->addMenu(submenu);
//            currentMenu = submenu;
//        } else if (item.startsWith("/")) {
//            QString submenuName = item.mid(1);
//            QMenu *submenu = new QMenu(submenuName, this);
//
//            //connectMenuSignals(this);
//
//            submenus[submenuName] = submenu;
//            currentMenu->addMenu(submenu);
//            currentMenu = submenu;
//        } else {
//            QAction *action = new QAction(item, this);
//            currentMenu->addAction(action);
//            connect(action, &QAction::triggered, this, [item] {
//                qDebug() << item << "triggered";
//            });
//        }
//    }
//}



void ContextMenu::openMenu() {
    if (isOpen_) {
        LogHandler::getInstance().info("Context menu is already open");
        return;
    }
    isOpen_ = true;

//    parentWindow->show();
//    parentWindow->raise();
//    parentWindow->activateWindow();

//    // Create an invisible parent window
//    QMainWindow* parentWindow = new QMainWindow();
//    parentWindow->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
//    parentWindow->setAttribute(Qt::WA_TransparentForMouseEvents);  // Make it transparent to mouse
//    parentWindow->setFocusPolicy(Qt::NoFocus);  // Prevent stealing focus
//    parentWindow->hide();  // Keep it hidden
//
//    // Create the context menu with the invisible parent window
//    ContextMenu* menu = new ContextMenu(parentWindow);

    this->setWindowFlags(Qt::Popup);
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus(Qt::ActiveWindowFocusReason);
//    this->setFocus();
    this->raise();
    this->activateWindow();
    this->move(QCursor::pos());

    QApplication::processEvents();
    this->exec();
}

void ContextMenu::closeMenu() {
    if (isOpen_) {
        LogHandler::getInstance().info("Context menu is already closed");
        return;
    }
    isOpen_ = false;
    this->hide();
}
