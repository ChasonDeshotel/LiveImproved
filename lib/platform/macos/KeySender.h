#ifndef KEY_SENDER_H
#define KEY_SENDER_H

class ApplicationManager;
class LogHandler;

class KeySender {
public:
    KeySender(ApplicationManager& appManager);
    ~KeySender();
    void sendKeyPress(int keyCode, int flags);

private:
    LogHandler* log_;
};

#endif
