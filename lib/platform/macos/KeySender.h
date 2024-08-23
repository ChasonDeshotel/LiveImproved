#ifndef KEY_SENDER_H
#define KEY_SENDER_H

class ApplicationManager;

class KeySender {
public:
    KeySender(ApplicationManager& appManager);
    ~KeySender();
    void sendKeypress(int keyCode, int flags);

private:
    ApplicationManager& app_;
};

#endif
