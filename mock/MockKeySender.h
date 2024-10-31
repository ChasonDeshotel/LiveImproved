#pragma once

#include "IKeySender.h"
#include <vector>

class MockKeySender : public IKeySender {
public:
    MockKeySender() = default;
    ~MockKeySender() override = default;

    void sendKeyPress(const EKeyPress& kp) override {
        m_sentKeyPresses.push_back(kp);
    }

    // Additional methods for testing
    const std::vector<EKeyPress>& getSentKeyPresses() const {
        return m_sentKeyPresses;
    }

    void reset() {
        m_sentKeyPresses.clear();
    }

private:
    std::vector<EKeyPress> m_sentKeyPresses;
};
