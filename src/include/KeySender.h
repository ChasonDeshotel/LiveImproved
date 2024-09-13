#ifndef KEY_SENDER_H
#define KEY_SENDER_H

class LogHandler;
class EKeyPress;

class KeySender {
public:
    static KeySender& getInstance() {
        static KeySender instance;
        return instance;
    }
    void sendKeyPress(const EKeyPress& kp);

private:
    KeySender();
    ~KeySender();

    KeySender(const KeySender&) = delete;
    KeySender& operator=(const KeySender&) = delete;

    LogHandler* log_;

    void sendKeyDown(const EKeyPress& kp);
    void sendKeyUp(const EKeyPress& kp);
    void sendIndividualKeyPress(const EKeyPress& kp);
    void sendModifiedKeyCombo(const EKeyPress& kp);
};

#endif
