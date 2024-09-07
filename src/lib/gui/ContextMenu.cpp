#include <QApplication>
#include <QMainWindow>
#include <QAction>
#include <QCursor>
#include <QDebug>
#include <QWidget>
#include <QWidgetAction>

#include "ApplicationManager.h"
#include "ContextMenu.h"
#include "LogHandler.h"
#include "PID.h"

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

    buildMenu(menuItems);
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


void ContextMenu::buildMenu(const QStringList &menuItems) {
    QMenu *currentMenu = this;  // Use QMenu* instead of ContextMenu*

    QMap<QString, QMenu*> submenus;

//    QLineEdit *searchBox = new QLineEdit(this);
//    searchBox->setPlaceholderText("Search...");
//
//    // Create a QWidgetAction to hold the QLineEdit
//    QWidgetAction *searchAction = new QWidgetAction(this);
//    searchAction->setDefaultWidget(searchBox);
//
//    this->addAction(searchAction);
//
//    connect(searchBox, &QLineEdit::textChanged, this, [this](const QString &text) {
//        // Implement search/filter logic here
//        qDebug() << "Searching for: " << text;
//    });


    for (const QString &item : menuItems) {
        if (item == "--") {
            currentMenu->addSeparator();
        } else if (item.startsWith("///")) {
            QString submenuName = item.mid(3);
            QMenu *submenu = new QMenu(submenuName, this);

            //connectMenuSignals(this);

            submenus[submenuName] = submenu;
            currentMenu->addMenu(submenu);
            currentMenu = submenu;
        } else if (item.startsWith("//")) {
            QString submenuName = item.mid(2);
            QMenu *submenu = new QMenu(submenuName, this);

            //connectMenuSignals(this);

            submenus[submenuName] = submenu;
            currentMenu->addMenu(submenu);
            currentMenu = submenu;
        } else if (item.startsWith("/")) {
            QString submenuName = item.mid(1);
            QMenu *submenu = new QMenu(submenuName, this);

            //connectMenuSignals(this);

            submenus[submenuName] = submenu;
            currentMenu->addMenu(submenu);
            currentMenu = submenu;
        } else {
            QAction *action = new QAction(item, this);
            currentMenu->addAction(action);
            connect(action, &QAction::triggered, this, [item] {
                qDebug() << item << "triggered";
            });
        }
    }
}



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
