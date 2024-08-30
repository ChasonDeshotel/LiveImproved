#ifndef GUI_SEARCH_BOX_H
#define GUI_SEARCH_BOX_H

#include <string>
#include <vector>
#include <memory>

#include "Plugin.h"

#include <QMouseEvent>
#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QString>

#include "LogHandler.h"

class ApplicationManager;
//class LogHandler;

class FocusedWidget : public QWidget {
    Q_OBJECT

public:
    FocusedWidget(QLineEdit* searchField, QWidget* parent = nullptr)
        : QWidget(parent), searchField_(searchField)
    {
        this->setWindowFlags(Qt::FramelessWindowHint);
    }

protected:
    void focusInEvent(QFocusEvent* event) override {
        QWidget::focusInEvent(event);
        if (searchField_) {
            searchField_->setFocus();
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            mousePressed_ = true;
            mouseStartPosition_ = event->globalPosition().toPoint();
            windowStartPosition_ = this->frameGeometry().topLeft();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (mousePressed_) {
            QPoint delta = event->globalPosition().toPoint() - mouseStartPosition_;
            this->move(windowStartPosition_ + delta);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            mousePressed_ = false;
        }
    }

    void keyPressEvent(QKeyEvent* event) override {
        QWidget::keyPressEvent(event);  // Call the base class implementation

        // Example: Handle specific key presses
        if (event->key() == Qt::Key_Escape) {
            // Handle the Escape key, e.g., close the widget
            close();
        } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            // Handle Enter/Return key
            // Add your logic here
        } else {
            // Handle other keys
        }

        // Log the key press event if needed
        LogHandler::getInstance().info("Key Pressed: " + std::to_string(event->key()));
    }

    // Handle key release events
    void keyReleaseEvent(QKeyEvent* event) override {
        QWidget::keyReleaseEvent(event);  // Call the base class implementation

        // Log the key release event if needed
        LogHandler::getInstance().info("Key Released: " + std::to_string(event->key()));
    }

private:
    QLineEdit* searchField_;
    bool mousePressed_;
    QPoint mouseStartPosition_;
    QPoint windowStartPosition_;
};

class GUISearchBox : public QWidget {
    Q_OBJECT

public:
    GUISearchBox(ApplicationManager& appManager);
    ~GUISearchBox();

    void closeSearchBox();
    void openSearchBox();

    bool isOpen() const;

    void setOptions(const std::vector<Plugin>& options);

    void setSearchText(const std::string text);
    std::string getSearchText() const;
    size_t getSearchTextLength() const;
    void clearSearchText();

    QWidget* getQtWidget() const;

    void handlePluginSelected(const Plugin& plugin);

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:

    void filterOptions(const QString &text);

    QLineEdit* searchField_;
    FocusedWidget* qtWidget_;
    QListWidget* optionsList_;

    bool isOpen_;

    std::string title;
    std::shared_ptr<LogHandler> log;

};

#endif
