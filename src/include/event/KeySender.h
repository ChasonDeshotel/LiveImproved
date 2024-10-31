#pragma once

#include "Types.h"
#include "IKeySender.h"

class KeySender : public IKeySender {
public:
    KeySender();
    ~KeySender() override;

    KeySender(const KeySender&) = delete;
    KeySender(KeySender &&) = delete;
    auto operator=(const KeySender&) -> KeySender& = delete;
    auto operator=(KeySender &&) -> KeySender& = delete;

    void sendKeyPress(const EKeyPress& kp) override;

private:
    void sendIndividualKeyPress(const EKeyPress& kp);
};
