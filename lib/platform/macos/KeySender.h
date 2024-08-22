#ifndef KEY_SENDER_H
#define KEY_SENDER_H

class KeySender {
public:
    KeySender();
    ~KeySender();
    void sendKeypress(int keyCode, int flags);
};

#endif
