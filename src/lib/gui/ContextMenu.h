#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include <QWidget>
#include <QMenu>
#include <QMap>
#include <QStringList>

class ContextMenu : public QMenu {
    Q_OBJECT

public:
    explicit ContextMenu(QWidget *parent = nullptr);

//protected:
//    void contextMenuEvent(QContextMenuEvent *event) override;
//
private:
    void buildMenu(const QStringList &menuItems);
    QMap<QString, QMenu*> submenus;
};

#endif
