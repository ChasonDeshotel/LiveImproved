#ifndef GUI_SEARCH_BOX_H
#define GUI_SEARCH_BOX_H

#include <string>
#include <vector>
#include <memory>

#include "FocusedWidget.h"

#include "Plugin.h"

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QString>

#include "LogHandler.h"

class ApplicationManager;

class CustomLineEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit CustomLineEdit(QWidget *parent = nullptr)
        : QLineEdit(parent) {
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (
              event->key() == Qt::Key_Up
              || event->key() == Qt::Key_Down
              || event->key() == Qt::Key_PageUp
              || event->key() == Qt::Key_PageDown
            
            
            ) {
            event->ignore();  // Let the event propagate to the parent
        } else {
            QLineEdit::keyPressEvent(event);  // Handle other keys normally
        }
    }
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

    void handlePluginSelected(QListWidgetItem* selectedItem);

    void setWindowGeometry();

protected:
    virtual void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    ApplicationManager& app_;

    void filterOptions(const QString &text);
    QListWidget* originalItems_;

    bool isOpen_;

    QLineEdit* searchField_;
    QWidget* qtWidget_;
    QListWidget* optionsList_;

    bool mousePressed_;
    QPoint mouseStartPosition_;
    QPoint windowStartPosition_;

    void onItemDoubleClicked(QListWidgetItem* item);

    std::string title;
    std::shared_ptr<LogHandler> log;

};

#endif
