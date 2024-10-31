#pragma once

class EKeyPress;

class IKeySender {
public:
    virtual ~IKeySender() = default;

    IKeySender(const IKeySender&) = default;
    IKeySender(IKeySender&&) = delete;
    auto operator=(const IKeySender&) -> IKeySender& = default;
    auto operator=(IKeySender&&) -> IKeySender& = delete;

    virtual void sendKeyPress(const EKeyPress& kp) = 0;

protected:
    IKeySender() = default;
};
