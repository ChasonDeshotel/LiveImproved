#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ActionHandler.h"
#include "MockConfigManager.h"
#include "MockWindowManager.h"
#include "MockPluginManager.h"
#include "MockEventHandler.h"
#include "MockLiveInterface.h"
#include "MockIPCQueue.h"
#include "LogGlobal.h"

#include <memory>


TEST_CASE("ActionHandler initialization") {
    auto mockConfigManager = std::make_shared<MockConfigManager>();
    auto mockWindowManager = std::make_shared<MockWindowManager>();
    auto mockPluginManager = std::make_shared<MockPluginManager>();
    auto mockIPCQueue = std::make_shared<MockIPCQueue>();
    auto mockEventHandler = std::make_shared<MockEventHandler>();
    auto mockLiveInterface = std::make_shared<MockLiveInterface>();

    ActionHandler actionHandler(
        [&]() { return mockPluginManager; },
        [&]() { return mockWindowManager; },
        [&]() { return mockConfigManager; },
        [&]() { return mockIPCQueue; },
        [&]() { return mockEventHandler; },
        [&]() { return mockLiveInterface; }
    );

    SUBCASE("Valid actions") {
        CHECK_NOTHROW(actionHandler.handleAction("closeFocusedPlugin"));
        CHECK_NOTHROW(actionHandler.handleAction("closeAllPlugins"));
        CHECK_NOTHROW(actionHandler.handleAction("openAllPlugins"));
        CHECK_NOTHROW(actionHandler.handleAction("tilePluginWindows"));
        CHECK_NOTHROW(actionHandler.handleAction("searchbox"));
    }

    SUBCASE("Invalid action") {
        CHECK_THROWS(actionHandler.handleAction("invalidAction"));
    }
}

TEST_CASE("ActionHandler handleKeyEvent") {
    auto mockConfigManager = std::make_shared<MockConfigManager>();
    auto mockWindowManager = std::make_shared<MockWindowManager>();
    auto mockPluginManager = std::make_shared<MockPluginManager>();
    auto mockIPCQueue = std::make_shared<MockIPCQueue>();
    auto mockEventHandler = std::make_shared<MockEventHandler>();
    auto mockLiveInterface = std::make_shared<MockLiveInterface>();

    ActionHandler actionHandler(
        [&]() { return pluginManager; },
        [&]() { return windowManager; },
        [&]() { return configManager; },
        [&]() { return mockIPCQueue; },
        [&]() { return mockEventHandler; },
        [&]() { return mockLiveInterface; }
    );

    EKeyPress testKeyPress;
    testKeyPress.key = "A";
    testKeyPress.cmd = false;
    testKeyPress.ctrl = false;
    testKeyPress.alt = false;
    testKeyPress.shift = false;

    CHECK(actionHandler.handleKeyEvent(testKeyPress));
}

TEST_CASE("ActionHandler handleAction with arguments") {
    auto mockConfigManager = std::make_shared<MockConfigManager>();
    auto mockWindowManager = std::make_shared<MockWindowManager>();
    auto mockPluginManager = std::make_shared<MockPluginManager>();
    auto mockIPCQueue = std::make_shared<MockIPCQueue>();
    auto mockEventHandler = std::make_shared<MockEventHandler>();
    auto mockLiveInterface = std::make_shared<MockLiveInterface>();

    ActionHandler actionHandler(
        [&]() { return pluginManager; },
        [&]() { return windowManager; },
        [&]() { return configManager; },
        [&]() { return mockIPCQueue; },
        [&]() { return mockEventHandler; },
        [&]() { return mockLiveInterface; }
    );

    CHECK_NOTHROW(actionHandler.handleAction("write-request.test_request"));
    CHECK_NOTHROW(actionHandler.handleAction("plugin.TestPlugin"));
}
