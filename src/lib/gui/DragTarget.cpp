#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QListView>
#include <QMimeData>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidget>

#include "LogHandler.h"
#include "ApplicationManager.h"
#include "DragTarget.h"

DragTarget::DragTarget(ApplicationManager& appManager)
    : title("drag")
    , app_(appManager)
    , isOpen_(false)
    , qtWidget_(this)
{

    LogHandler::getInstance().info("Creating DragTargetWindowController");

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    this->setAcceptDrops(true);

    setWindowGeometry();

    QVBoxLayout *layout = new QVBoxLayout(this);

    this->setLayout(layout);

}

DragTarget::~DragTarget() {}

QWidget* DragTarget::getQtWidget() const {
    return qtWidget_;
}

bool DragTarget::isOpen() const {
    return isOpen_;
}

void DragTarget::setWindowGeometry() {
    // TODO: store conf if the window has been moved
    // or resized then override these with values
    // from conf
    //
    // TODO: works with dual monitors?
    // works with two ableton windows open?
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenWidth = screen->geometry().width();
    int screenHeight = screen->geometry().height();

    ERect liveBounds = app_.getEventHandler()->getLiveBoundsRect();
    int widgetWidth = 600;  // Width of the widget
    int widgetHeight = 300; // Height of the widget
    int xPos = liveBounds.x + (liveBounds.width - widgetWidth) / 2;
    int yPos = liveBounds.y + (liveBounds.height - widgetHeight) / 2;

    xPos = std::max(0, std::min(xPos, screenWidth - widgetWidth));
    yPos = std::max(0, std::min(yPos, screenHeight - widgetHeight));

    this->setGeometry(xPos, yPos, widgetWidth, widgetHeight);
}

void DragTarget::openWindow() {
    if (isOpen_) {
        LogHandler::getInstance().info("Drag box is already open");
        return;
    }

    setWindowGeometry();
    qtWidget_->show();
    isOpen_ = true;
    qtWidget_->raise();        // Raises the widget to the top of the stack
    qtWidget_->activateWindow();
    ApplicationManager::getInstance().getEventHandler()->focusApplication(PID::getInstance().appPID());
}

void DragTarget::closeEvent(QCloseEvent* event) {
    close();
    QWidget::closeEvent(event);
    ApplicationManager::getInstance().getEventHandler()->focusApplication(PID::getInstance().appPID());
}

void DragTarget::closeWindow() {
    if (!isOpen_) {
        LogHandler::getInstance().info("Drag box is not open");
        return;
    }
    qtWidget_->hide();
    isOpen_ = false;

    ApplicationManager::getInstance().getEventHandler()->focusApplication(PID::getInstance().livePID());
}

void DragTarget::dragEnterEvent(QDragEnterEvent *event) {
    const QMimeData* mimeData = event->mimeData();

    // Convert all MIME data to a string for logging
    QString mimeDataString;
    QStringList formats = mimeData->formats();
    for (const QString &format : formats) {
        mimeDataString += "Format: " + format + "\n";
        QByteArray data = mimeData->data(format);
        mimeDataString += "Data: " + QString(data.toHex()) + "\n\n"; // Convert the data to a hex string
    }

    LogHandler::getInstance().info("Drag enter: " + mimeDataString.toStdString());

    if (event->mimeData()->hasUrls()) {  // Example: checking for files being dragged
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DragTarget::dropEvent(QDropEvent *event) {
    const QMimeData* mimeData = event->mimeData();

    // Convert all MIME data to a string for logging
    QString mimeDataString;
    QStringList formats = mimeData->formats();
    for (const QString &format : formats) {
        mimeDataString += "Format: " + format + "\n";
        QByteArray data = mimeData->data(format);
        mimeDataString += "Data: " + QString(data.toHex()) + "\n\n"; // Convert the data to a hex string
    }
    LogHandler::getInstance().info("Dropped: " + mimeDataString.toStdString());


    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        foreach (QUrl url, urls) {
            QString fileName = url.toLocalFile();
            // Process the file or data
            LogHandler::getInstance().info("Dropped file: " + fileName.toStdString());
        }
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DragTarget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed_ = true;
        mouseStartPosition_ = event->globalPosition().toPoint();
        windowStartPosition_ = this->frameGeometry().topLeft();
    }
}

void DragTarget::mouseMoveEvent(QMouseEvent* event) {
    if (mousePressed_) {
        QPoint delta = event->globalPosition().toPoint() - mouseStartPosition_;
        this->move(windowStartPosition_ + delta);
    }
}

void DragTarget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed_ = false;
    }
}

void DragTarget::keyPressEvent(QKeyEvent* event) {
    QWidget::keyPressEvent(event);

    if (event->modifiers() & Qt::ShiftModifier) {
        LogHandler::getInstance().info("Shift modifier is pressed");
    }
    if (event->modifiers() & Qt::ControlModifier) {
        LogHandler::getInstance().info("Control modifier is pressed");
    }
    if (event->modifiers() & Qt::AltModifier) {
        LogHandler::getInstance().info("Alt modifier is pressed");
    }
    if (event->modifiers() & Qt::MetaModifier) {
        LogHandler::getInstance().info("Meta (Command) modifier is pressed");
    }

    if (event->key() == Qt::Key_Escape) {
        close();
    }

    //} else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
     //   LogHandler::getInstance().info("enter pressed");
    //} else {
    //    LogHandler::getInstance().info("KEY pressed: " + std::to_string(event->key()));
   // }


    // Log the key press event if needed
    LogHandler::getInstance().info("Key Pressed: " + std::to_string(event->key()));
}

// Handle key release events
void DragTarget::keyReleaseEvent(QKeyEvent* event) {
    QWidget::keyReleaseEvent(event);  // Call the base class implementation

    // Log the key release event if needed
    //LogHandler::getInstance().info("Key Released: " + std::to_string(event->key()));
}
