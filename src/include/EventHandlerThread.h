#ifndef EVENT_HANDLER_THREAD_H
#define EVENT_HANDLER_THREAD_H

#include <QThread>
#include "EventHandler.h"

class EventHandlerThread : public QThread {
    Q_OBJECT

public:
    EventHandlerThread(
        EventHandler* handler_
        , QObject *parent = nullptr
    );
    ~EventHandlerThread();  // Ensure virtual destructor is declared

protected:
    void run() override;

private:
    EventHandler* handler_;

};

#endif
