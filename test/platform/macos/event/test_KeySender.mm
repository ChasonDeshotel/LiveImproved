#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "KeySender.h"
#include "LogGlobal.h"

#include <memory>
#include <iostream>

TEST_CASE("global setup") {
    initializeLogger();
}

TEST_CASE("KeySender initialization") {
    std::cout << "Starting KeySender initialization test" << std::endl;

    SUBCASE("KeySender creation") {
        CHECK_NOTHROW(KeySender());
    }
}

TEST_CASE("KeySender sendKeyPress") {
    std::cout << "Starting KeySender sendKeyPress test" << std::endl;

    KeySender keySender;

    SUBCASE("Valid key press") {
        EKeyPress keyPress;
        keyPress.key = "A";
        keyPress.cmd = false;
        keyPress.ctrl = false;
        keyPress.alt = false;
        keyPress.shift = false;

        CHECK_NOTHROW(keySender.sendKeyPress(keyPress));
    }

    SUBCASE("Key press with modifiers") {
        EKeyPress keyPress;
        keyPress.key = "C";
        keyPress.cmd = true;
        keyPress.ctrl = false;
        keyPress.alt = false;
        keyPress.shift = false;

        CHECK_NOTHROW(keySender.sendKeyPress(keyPress));
    }

    SUBCASE("Invalid key") {
        EKeyPress keyPress;
        keyPress.key = "InvalidKey";
        keyPress.cmd = false;
        keyPress.ctrl = false;
        keyPress.alt = false;
        keyPress.shift = false;

        CHECK_NOTHROW(keySender.sendKeyPress(keyPress));
    }
}

TEST_CASE("KeySender helper functions") {
    std::cout << "Starting KeySender helper functions test" << std::endl;

    SUBCASE("toLower function") {
        CHECK(toLower("HELLO") == "hello");
        CHECK(toLower("World") == "world");
        CHECK(toLower("123") == "123");
    }

    SUBCASE("getKeyCode function") {
        CHECK(getKeyCode("a").has_value());
        CHECK(getKeyCode("shift").has_value());
        CHECK_FALSE(getKeyCode("invalidkey").has_value());
    }

    SUBCASE("getEventFlags function") {
        EKeyPress kp;
        kp.cmd = true;
        kp.ctrl = false;
        kp.alt = true;
        kp.shift = false;

        CGEventFlags flags = getEventFlags(kp);
        CHECK((flags & kCGEventFlagMaskCommand) != 0);
        CHECK((flags & kCGEventFlagMaskControl) == 0);
        CHECK((flags & kCGEventFlagMaskAlternate) != 0);
        CHECK((flags & kCGEventFlagMaskShift) == 0);
    }
}
