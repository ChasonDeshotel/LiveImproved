#include "ContextMenu.h"
#include <QAction>
#include <QCursor>
#include <QDebug>

ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent) {

    // Set the window flag to keep the menu on top
    setWindowFlag(Qt::WindowStaysOnTopHint);

    QStringList menuItems = {
        "/Menu1", "Item1", "Item2", "--", "Item3", "--",
        "/Menu2", "Foo", "Bar", "//Submenu2", "Test", "///Submenu2Submenu", "AnotherItem"
    };

    buildMenu(menuItems);
}

void ContextMenu::buildMenu(const QStringList &menuItems) {
    QMenu *currentMenu = this;  // Use QMenu* instead of ContextMenu*

    QMap<QString, QMenu*> submenus;

    for (const QString &item : menuItems) {
        if (item == "--") {
            currentMenu->addSeparator();
        } else if (item.startsWith("///")) {
            QString submenuName = item.mid(3);
            QMenu *submenu = new QMenu(submenuName, this);
            submenus[submenuName] = submenu;
            currentMenu->addMenu(submenu);
            currentMenu = submenu;
        } else if (item.startsWith("//")) {
            QString submenuName = item.mid(2);
            QMenu *submenu = new QMenu(submenuName, this);
            submenus[submenuName] = submenu;
            currentMenu->addMenu(submenu);
            currentMenu = submenu;
        } else if (item.startsWith("/")) {
            QString submenuName = item.mid(1);
            QMenu *submenu = new QMenu(submenuName, this);
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

