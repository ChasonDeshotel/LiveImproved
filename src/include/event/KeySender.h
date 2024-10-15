#pragma once

#include "Types.h"

class KeySender {
public:
    KeySender();
    KeySender(const KeySender&) = delete;
    KeySender(KeySender &&) = delete;
    auto operator=(const KeySender&) -> KeySender& = delete;
    auto operator=(KeySender &&) -> KeySender& = delete;
    ~KeySender();

    static auto getInstance() -> KeySender& {
        static KeySender instance;
        return instance;
    }
    void sendKeyPress(const EKeyPress& kp);

private:
    void sendIndividualKeyPress(const EKeyPress& kp);
};
