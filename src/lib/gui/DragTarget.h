#ifndef DRAG_TARGET_H
#define DRAG_TARGET_H

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QString>

#include "LogHandler.h"

class ApplicationManager;

class DragTarget : public QWidget {
    Q_OBJECT

public:
    DragTarget(ApplicationManager& appManager);
    ~DragTarget();

    void closeWindow();
    void openWindow();

    bool isOpen() const;

    QWidget* getQtWidget() const;

    void setWindowGeometry();

protected:

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    virtual void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    ApplicationManager& app_;

    bool isOpen_;

    QWidget* qtWidget_;

    bool mousePressed_;
    QPoint mouseStartPosition_;
    QPoint windowStartPosition_;

    std::string title;
    std::shared_ptr<LogHandler> log;

};

#endif
