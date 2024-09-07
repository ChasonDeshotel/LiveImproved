#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include <QWidget>
#include <QMenu>
#include <QMap>
#include <QStringList>
#include <QKeyEvent>

class ContextMenu : public QMenu {
    Q_OBJECT

public:
    explicit ContextMenu(QWidget *parent = nullptr);

    void openMenu();
    void closeMenu();
    bool isOpen() const;
    QMenu* getActiveMenu() const;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    // void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void buildMenu(const QStringList &menuItems);
    QMap<QString, QMenu*> submenus;

//    QAction* findNextAction(QAction* currentAction, bool forward);

    bool isOpen_;

//    QMenu* activeMenu_;

//    void connectMenuSignals(QMenu *menu);
};

#endif
