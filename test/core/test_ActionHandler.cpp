#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "MockWindowManager.h"
#include "MockPluginManager.h"
#include "MockEventHandler.h"
#include "MockLiveInterface.h"
#include "MockIPCQueue.h"
#include "MockKeySender.h"
#include "LogGlobal.h"

TEST_CASE("global setup") {
    initializeLogger();
}

#include <memory>
#include <iostream>

#include "PathManager.h"
auto pathManager = PathManager();
auto configFilePath = pathManager.config();
auto configMenuPath = pathManager.configMenu();

// Helper function to create ActionHandler
std::unique_ptr<ActionHandler> createActionHandler(
    std::shared_ptr<MockPluginManager>& mockPluginManager,
    std::shared_ptr<MockWindowManager>& mockWindowManager,
    std::shared_ptr<ConfigManager>& mockConfigManager,
    std::shared_ptr<MockIPCQueue>& mockIPCQueue,
    std::shared_ptr<MockEventHandler>& mockEventHandler,
    std::shared_ptr<MockLiveInterface>& mockLiveInterface,
    std::shared_ptr<MockKeySender>& mockKeySender
) {
    std::cout << "Creating ActionHandler..." << std::endl;
    return std::make_unique<ActionHandler>(
        [&]() { return mockPluginManager; },
        [&]() { return mockWindowManager; },
        [&]() { return mockConfigManager; },
        [&]() { return mockIPCQueue; },
        [&]() { return mockEventHandler; },
        [&]() { return mockLiveInterface; },
        [&]() { return mockKeySender; }
    );
}

TEST_CASE("ActionHandler initialization") {
    std::cout << "Starting ActionHandler initialization test" << std::endl;

    auto mockConfigManager = std::make_shared<ConfigManager>(configFilePath);
    auto mockWindowManager = std::make_shared<MockWindowManager>();
    auto mockPluginManager = std::make_shared<MockPluginManager>();
    auto mockIPCQueue = std::make_shared<MockIPCQueue>();
    auto mockEventHandler = std::make_shared<MockEventHandler>();
    auto mockLiveInterface = std::make_shared<MockLiveInterface>();
    auto mockKeySender = std::make_shared<MockKeySender>();

    REQUIRE(mockConfigManager != nullptr);
    REQUIRE(mockWindowManager != nullptr);
    REQUIRE(mockPluginManager != nullptr);
    REQUIRE(mockIPCQueue != nullptr);
    REQUIRE(mockEventHandler != nullptr);
    REQUIRE(mockLiveInterface != nullptr);
    REQUIRE(mockKeySender != nullptr);

    auto actionHandler = createActionHandler(
        mockPluginManager, mockWindowManager, mockConfigManager,
        mockIPCQueue, mockEventHandler, mockLiveInterface, mockKeySender
    );

    REQUIRE(actionHandler != nullptr);

    SUBCASE("Valid actions") {
        std::cout << "Testing valid actions" << std::endl;
        CHECK_NOTHROW(actionHandler->handleAction("closeFocusedPlugin"));
        CHECK_NOTHROW(actionHandler->handleAction("closeAllPlugins"));
        CHECK_NOTHROW(actionHandler->handleAction("openAllPlugins"));
        CHECK_NOTHROW(actionHandler->handleAction("tilePluginWindows"));
        CHECK_NOTHROW(actionHandler->handleAction("searchbox"));
    }

    // TODO throw if invalid action or something
    //SUBCASE("Invalid action") {
    //    std::cout << "Testing invalid action" << std::endl;
    //    CHECK_THROWS_AS(actionHandler->handleAction("invalidAction"), std::runtime_error);
    //}
}

TEST_CASE("ActionHandler handleKeyEvent") {
    std::cout << "Starting ActionHandler handleKeyEvent test" << std::endl;

    auto mockConfigManager = std::make_shared<ConfigManager>(configFilePath);
    auto mockWindowManager = std::make_shared<MockWindowManager>();
    auto mockPluginManager = std::make_shared<MockPluginManager>();
    auto mockIPCQueue = std::make_shared<MockIPCQueue>();
    auto mockEventHandler = std::make_shared<MockEventHandler>();
    auto mockLiveInterface = std::make_shared<MockLiveInterface>();

    auto actionHandler = createActionHandler(
        mockPluginManager, mockWindowManager, mockConfigManager,
        mockIPCQueue, mockEventHandler, mockLiveInterface
    );

    REQUIRE(actionHandler != nullptr);

    EKeyPress testKeyPress;
    testKeyPress.key = "A";
    testKeyPress.cmd = false;
    testKeyPress.ctrl = false;
    testKeyPress.alt = false;
    testKeyPress.shift = false;

    CHECK_MESSAGE(actionHandler->handleKeyEvent(testKeyPress), "handleKeyEvent should return true for a valid key press");
}

TEST_CASE("ActionHandler handleAction with arguments") {
    std::cout << "Starting ActionHandler handleAction with arguments test" << std::endl;

    auto mockConfigManager = std::make_shared<ConfigManager>(configFilePath);
    auto mockWindowManager = std::make_shared<MockWindowManager>();
    auto mockPluginManager = std::make_shared<MockPluginManager>();
    auto mockIPCQueue = std::make_shared<MockIPCQueue>();
    auto mockEventHandler = std::make_shared<MockEventHandler>();
    auto mockLiveInterface = std::make_shared<MockLiveInterface>();

    auto actionHandler = createActionHandler(
        mockPluginManager, mockWindowManager, mockConfigManager,
        mockIPCQueue, mockEventHandler, mockLiveInterface
    );

    REQUIRE(actionHandler != nullptr);

    SUBCASE("write-request action") {
        CHECK_NOTHROW_MESSAGE(actionHandler->handleAction("write-request.test_request"), "write-request action should not throw");
    }

    SUBCASE("plugin action") {
        CHECK_NOTHROW_MESSAGE(actionHandler->handleAction("plugin.TestPlugin"), "plugin action should not throw");
    }
}
