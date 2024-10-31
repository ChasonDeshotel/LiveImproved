#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "IKeySender.h"
#include "KeySender.h"
#include "Types.h"
#include <memory>

class MockKeySender : public IKeySender {
public:
    void sendKeyPress(const EKeyPress& kp) override {
        lastKeyPress = kp;
    }

    EKeyPress lastKeyPress;
};

TEST_CASE("IKeySender tests") {
    auto keySender = std::make_unique<MockKeySender>();

    SUBCASE("sendKeyPress test") {
        EKeyPress kp = {.shift = false, .ctrl = false, .cmd = true, .alt = true, .key = "a"};
        keySender->sendKeyPress(kp);

        CHECK(keySender->lastKeyPress.key == "a");
        CHECK(keySender->lastKeyPress.cmd == true);
        CHECK(keySender->lastKeyPress.ctrl == false);
        CHECK(keySender->lastKeyPress.alt == true);
        CHECK(keySender->lastKeyPress.shift == false);
    }

    SUBCASE("Multiple key presses") {
        EKeyPress kp1 = {.shift = true, .ctrl = false, .cmd = false, .alt = false, .key = "b"};
        keySender->sendKeyPress(kp1);

        CHECK(keySender->lastKeyPress.key == "b");
        CHECK(keySender->lastKeyPress.cmd == false);
        CHECK(keySender->lastKeyPress.ctrl == false);
        CHECK(keySender->lastKeyPress.alt == false);
        CHECK(keySender->lastKeyPress.shift == true);

        EKeyPress kp2 = {.shift = false, .ctrl = false, .cmd = false, .alt = false, .key = "enter"};
        keySender->sendKeyPress(kp2);

        CHECK(keySender->lastKeyPress.key == "enter");
        CHECK(keySender->lastKeyPress.cmd == false);
        CHECK(keySender->lastKeyPress.ctrl == false);
        CHECK(keySender->lastKeyPress.alt == false);
        CHECK(keySender->lastKeyPress.shift == false);
    }
}

TEST_CASE("KeySender implementation tests") {
    auto keySender = std::make_unique<KeySender>();

    SUBCASE("sendKeyPress doesn't throw") {
        EKeyPress kp = {.shift = false, .ctrl = false, .cmd = true, .alt = true, .key = "a"};
        CHECK_NOTHROW(keySender->sendKeyPress(kp));
    }

    // Note: We can't easily test the actual key press simulation in a unit test.
    // These tests mainly ensure that the method can be called without errors.
}
