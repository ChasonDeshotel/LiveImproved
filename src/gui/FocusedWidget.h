#ifndef FOCUSED_WIDGET_H
#define FOCUSED_WIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPoint>
#include "LogHandler.h"

class FocusedWidget : public QWidget {
    Q_OBJECT

public:
    FocusedWidget(QLineEdit* searchField, QWidget* parent = nullptr);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    QLineEdit* searchField_;
    bool mousePressed_;
    QPoint mouseStartPosition_;
    QPoint windowStartPosition_;

signals:
    void requestClose();

};

#endif
