#include "FocusedWidget.h"

FocusedWidget::FocusedWidget(QLineEdit* searchField, QWidget* parent)
    : QWidget(parent), searchField_(searchField), mousePressed_(false) {
    this->setWindowFlags(Qt::FramelessWindowHint);
}

void FocusedWidget::focusInEvent(QFocusEvent* event) {
    QWidget::focusInEvent(event);
    if (searchField_) {
        searchField_->setFocus();
    }
}

void FocusedWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed_ = true;
        mouseStartPosition_ = event->globalPosition().toPoint();
        windowStartPosition_ = this->frameGeometry().topLeft();
    }
}

void FocusedWidget::mouseMoveEvent(QMouseEvent* event) {
    if (mousePressed_) {
        QPoint delta = event->globalPosition().toPoint() - mouseStartPosition_;
        this->move(windowStartPosition_ + delta);
    }
}

void FocusedWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed_ = false;
    }
}

void FocusedWidget::keyPressEvent(QKeyEvent* event) {
    QWidget::keyPressEvent(event);  // Call the base class implementation

    // Example: Handle specific key presses
    if (event->key() == Qt::Key_Escape) {
        // Handle the Escape key, e.g., close the widget
        emit requestClose();
    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        // Handle Enter/Return key
        // Add your logic here
    }

    // Log the key press event if needed
    LogHandler::getInstance().info("Key Pressed: " + std::to_string(event->key()));
}

// Handle key release events
void FocusedWidget::keyReleaseEvent(QKeyEvent* event) {
    QWidget::keyReleaseEvent(event);  // Call the base class implementation

    // Log the key release event if needed
    LogHandler::getInstance().info("Key Released: " + std::to_string(event->key()));
}
