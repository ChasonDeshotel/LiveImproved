#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "WindowManager.h"
#include "PluginManager.h"
#include "EventHandler.h"
#include "ILiveInterface.h"
#include "IIPCQueue.h"
#include "MockIPCQueue.h"
#include "LogGlobal.h"

#include <memory>


TEST_CASE("ActionHandler initialization") {
    auto configManager = std::make_shared<MockConfigManager>();
    auto windowManager = std::make_shared<MockWindowManager>();
    auto pluginManager = std::make_shared<MockPluginManager>();
    auto ipcQueue = std::make_shared<MockIPCQueue>();
    auto eventHandler = std::make_shared<MockEventHandler>();
    auto liveInterface = std::make_shared<MockLiveInterface>();

    ActionHandler actionHandler(
        [&]() { return pluginManager; },
        [&]() { return windowManager; },
        [&]() { return configManager; },
        [&]() { return ipcQueue; },
        [&]() { return eventHandler; },
        [&]() { return liveInterface; }
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
    auto configManager = std::make_shared<MockConfigManager>();
    auto windowManager = std::make_shared<MockWindowManager>();
    auto pluginManager = std::make_shared<MockPluginManager>();
    auto ipcCore = std::make_shared<MockIPCCore>();
    auto eventHandler = std::make_shared<MockEventHandler>();
    auto liveInterface = std::make_shared<MockLiveInterface>();

    ActionHandler actionHandler(
        [&]() { return pluginManager; },
        [&]() { return windowManager; },
        [&]() { return configManager; },
        [&]() { return ipcCore; },
        [&]() { return eventHandler; },
        [&]() { return liveInterface; }
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
    auto configManager = std::make_shared<MockConfigManager>();
    auto windowManager = std::make_shared<MockWindowManager>();
    auto pluginManager = std::make_shared<MockPluginManager>();
    auto ipcCore = std::make_shared<MockIPCCore>();
    auto eventHandler = std::make_shared<MockEventHandler>();
    auto liveInterface = std::make_shared<MockLiveInterface>();

    ActionHandler actionHandler(
        [&]() { return pluginManager; },
        [&]() { return windowManager; },
        [&]() { return configManager; },
        [&]() { return ipcCore; },
        [&]() { return eventHandler; },
        [&]() { return liveInterface; }
    );

    CHECK_NOTHROW(actionHandler.handleAction("write-request.test_request"));
    CHECK_NOTHROW(actionHandler.handleAction("plugin.TestPlugin"));
}
