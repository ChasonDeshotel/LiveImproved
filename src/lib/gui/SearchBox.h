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
//class LogHandler;

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
