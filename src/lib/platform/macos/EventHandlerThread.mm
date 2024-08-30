#include <QApplication>
#include <QThread>
#include "ApplicationManager.h"

EventHandlerThread::EventHandlerThread(EventHandler* handler, QObject *parent)
    : QThread(parent), handler_(handler) {}

EventHandlerThread::~EventHandlerThread() {}

void EventHandlerThread::run() {
    @autoreleasepool {
        ApplicationManager::getInstance().getEventHandler()->init();
        CFRunLoopRun(); // Start the event tap run loop in this thread
    }
}

