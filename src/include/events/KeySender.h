#pragma once

#include "Types.h"

class KeySender {
public:
    KeySender();
    ~KeySender();

    static KeySender& getInstance() {
        static KeySender instance;
        return instance;
    }
    void sendKeyPress(const EKeyPress& kp);

private:

    KeySender(const KeySender&) = delete;
    KeySender& operator=(const KeySender&) = delete;

    //void sendKeyDown(const EKeyPress& kp);
    //void sendKeyUp(const EKeyPress& kp);
    void sendIndividualKeyPress(const EKeyPress& kp);
    //void sendModifiedKeyCombo(const EKeyPress& kp);
};
