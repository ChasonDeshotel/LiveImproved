#define BOOST_TEST_MODULE KeyMapper
#include <boost/test/included/unit_test.hpp>
#include "KeyMapper.h"

BOOST_AUTO_TEST_SUITE(KeyMapperTests)

class LogHandler {
public:
    void info(const std::string& message) {
        BOOST_TEST_MESSAGE(message);  // Use Boost's test message for logging
    }

    static LogHandler& getInstance() {
        static LogHandler instance;
        return instance;
    }

private:
    // Private constructor to enforce singleton pattern
    LogHandler() = default;
    LogHandler(const LogHandler&) = delete;
    LogHandler& operator=(const LogHandler&) = delete;
};


class ApplicationManager {
public:
    ApplicationManager() 
        : mockLogHandler(LogHandler::getInstance()) {}

    LogHandler* getLogHandler() {
        return &mockLogHandler;
    }

private:
    LogHandler& mockLogHandler;  // Reference to the singleton instance
};

BOOST_AUTO_TEST_CASE(valid_hotkeys) {
    BOOST_TEST_MESSAGE("test");
//    KeyMapper keyMapper = createKeyMapper();
//
//    // Test valid hotkeys
//    std::string keypressString = "cmd+shift+b";
//    KeyPress kp = keyMapper.processKeyPress(keypressString);
//
//    BOOST_TEST(keyMapper.isValid());
//    BOOST_TEST(kp.key == 'b');
//    BOOST_TEST(kp.modifiers.size() == 2);  // cmd and shift
//    BOOST_TEST(kp.modifiers[0] == Modifier::Cmd);
//    BOOST_TEST(kp.modifiers[1] == Modifier::Shift);
}

//BOOST_AUTO_TEST_CASE(invalid_hotkeys) {
//    KeyMapper keyMapper = createKeyMapper();
//
//    // Test invalid hotkeys
//    std::string keypressString = "cmd+shift+";
//    KeyPress kp = keyMapper.processKeyPress(keypressString);
//
//    BOOST_TEST(!keyMapper.isValid());
//}
//
//BOOST_AUTO_TEST_CASE(empty_keypress) {
//    KeyMapper keyMapper = createKeyMapper();
//
//    // Test empty keypress
//    std::string keypressString = "";
//    KeyPress kp = keyMapper.processKeyPress(keypressString);
//
//    BOOST_TEST(!keyMapper.isValid());
//}
//
//BOOST_AUTO_TEST_CASE(single_keypress) {
//    KeyMapper keyMapper = createKeyMapper();
//
//    // Test single keypress without modifiers
//    std::string keypressString = "a";
//    KeyPress kp = keyMapper.processKeyPress(keypressString);
//
//    BOOST_TEST(keyMapper.isValid());
//    BOOST_TEST(kp.key == 'a');
//    BOOST_TEST(kp.modifiers.empty());
//}

BOOST_AUTO_TEST_SUITE_END()
