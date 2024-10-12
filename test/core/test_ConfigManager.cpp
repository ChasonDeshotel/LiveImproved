#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest/doctest.h"
#include "ConfigManager.h"
#include "DependencyContainer.h"
#include "MockLogHandler.h"

#include <fstream>
#include <optional>
#include <iostream>

DependencyContainer& container = DependencyContainer::getInstance();
std::shared_ptr<ILogHandler> logger = nullptr;

TEST_CASE("Setup") {
    CAPTURE("Setting up test environment");
    logger = std::make_shared<MockLogHandler>();
    container.registerFactory<ILogHandler>(
        [&](DependencyContainer&) { return logger; },
        DependencyContainer::Lifetime::Singleton
    );
    logger = container.resolve<ILogHandler>();

    std::string configFile = "test_config.yaml";
    container.registerFactory<ConfigManager>(
        [configFile](DependencyContainer& c) {
            return std::make_shared<ConfigManager>(configFile);
        },
        DependencyContainer::Lifetime::Transient
    );

    //logger->setLogLevel(LogLevel::LOG_DEBUG);  // Set log level to DEBUG for tests
}

// Helper function to create a temporary config file
std::string createTempConfigFile(const std::string& content) {
    std::string tempFileName = "temp_config.yaml";
    std::ofstream outFile(tempFileName);

    if (!outFile.is_open()) {
        std::cerr << "Failed to create temp file: " << tempFileName << std::endl;
        return "";
    }

    outFile << content;
    outFile.close();

    return tempFileName;
}

// Helper function to delete the temporary config file
void deleteTempConfigFile(const std::string &filename) {
    std::remove(filename.c_str());
}

// Custom comparison function for std::optional
template<typename T>
void check_optional_equal(const std::optional<T>& opt, const T& value) {
    REQUIRE(opt.has_value());
    CHECK(opt.value() == value);
}

class ConfigManagerTestFixture {
public:
    ConfigManagerTestFixture() {
        std::cout << "ConfigManagerTestFixture constructor called" << std::endl;
    }

    ~ConfigManagerTestFixture() {
        if (!configFile.empty()) {
            deleteTempConfigFile(configFile);
        }
    }

    std::string createConfigFile(const std::string& content) {
        configFile = "test_config.yaml";
        std::ofstream outFile(configFile);
        if (!outFile.is_open()) {
            std::cerr << "Failed to create temp file: " << configFile << std::endl;
            return "";
        }
        outFile << content;
        outFile.close();
        return configFile;
    }

private:
    std::string configFile;
};

// Note: We don't need to add more tests specifically for the ConfigManagerTestFixture
// because the existing "ConfigManager - Load Config" test case already uses this fixture.
// The fixture is properly set up and torn down for each test case that uses it.

TEST_CASE_FIXTURE(ConfigManagerTestFixture, "ConfigManager - Load Config") {
    CAPTURE("Starting ConfigManager - Load Config test");

    std::string configContent = R"(
# Keyboard shortcuts
remap:
  ctrl+d: cmd+a
  ctrl+2: shift+f
  d: delete
  cmd+b: cmd+d, cmd+d, cmd+d, cmd+d
  alt+a: plugin.Serum

# Plugins
rename-plugins:
  Serum: Daddy Duda's Special Synth

remove-plugins:
  - TerribleSynth
  - I'mTooLazyToUninstallSynth

# Quick shortcuts menu
shortcuts:
  - key: /location/to/shortcut/1
  - other: /location/to/shortcut/2

# Application behavior
init:
  retries: 10

window:
  search: 100,200,500,500
  preferences: 50,150,200,300
)";
    std::string configFile = "test_config.yaml";
    std::ofstream outFile(configFile);
    outFile << configContent;
    outFile.close();

    // Create a new ConfigManager instance with the temporary config file
    auto configManager = std::make_shared<ConfigManager>(configFile);
    configManager->loadConfig();

    SUBCASE("Check init retries") {
        CHECK_MESSAGE(configManager->getInitRetries() == 10, "Init retries should be 10");
    }

    SUBCASE("Check remap") {
        auto remap = configManager->getRemap();

        EKeyPress kpFrom;
        kpFrom.key = "d";
        EKeyPress kpTo;
        kpTo.key = "delete";
        EMacro macro;
        macro.addKeyPress(kpTo);

        CHECK_MESSAGE(remap.find(kpFrom) != remap.end(), "Remap should contain 'd' key");
        CHECK_MESSAGE(remap[kpFrom] == macro, "Remap 'd' should map to 'delete'");

        Action actionTo("plugin", "Serum");
        EKeyPress actionKPFrom;
        actionKPFrom.alt = true;
        actionKPFrom.key = "a";
        EMacro actionMacro;
        actionMacro.addAction(actionTo);

        CHECK_MESSAGE(remap.find(actionKPFrom) != remap.end(), "Remap should contain 'alt+a' key");
        CHECK_MESSAGE(remap[actionKPFrom] == actionMacro, "Remap 'alt+a' should map to plugin.Serum action");
    }

    SUBCASE("Check rename plugins") {
        auto renamePlugins = configManager->getRenamePlugins();
        CHECK_MESSAGE(!renamePlugins.empty(), "Rename plugins should not be empty");
        CHECK_MESSAGE(renamePlugins["Serum"] == "Daddy Duda's Special Synth", "Serum should be renamed to 'Daddy Duda's Special Synth'");
    }

    SUBCASE("Check remove plugins") {
        auto removePlugins = configManager->getRemovePlugins();
        CHECK_MESSAGE(!removePlugins.empty(), "Remove plugins should not be empty");
        CHECK_MESSAGE(removePlugins.size() == 2, "Remove plugins should contain 2 items");
        CHECK_MESSAGE(removePlugins[0] == "TerribleSynth", "First remove plugin should be 'TerribleSynth'");
        CHECK_MESSAGE(removePlugins[1] == "I'mTooLazyToUninstallSynth", "Second remove plugin should be 'I'mTooLazyToUninstallSynth'");
    }

    SUBCASE("Check window settings") {
        auto windowSettings = configManager->getWindowSettings();
        CHECK_MESSAGE(!windowSettings.empty(), "Window settings should not be empty");
        CHECK_MESSAGE(windowSettings["search"] == "100,200,500,500", "Search window settings should be '100,200,500,500'");
        CHECK_MESSAGE(windowSettings["preferences"] == "50,150,200,300", "Preferences window settings should be '50,150,200,300'");
    }

    SUBCASE("Check shortcuts") {
        auto shortcuts = configManager->getShortcuts();
        CHECK_MESSAGE(!shortcuts.empty(), "Shortcuts should not be empty");
        REQUIRE_MESSAGE(shortcuts.size() == 2, "Shortcuts should contain 2 items");
        CHECK_MESSAGE(shortcuts[0].at("key") == "/location/to/shortcut/1", "First shortcut should be '/location/to/shortcut/1'");
        CHECK_MESSAGE(shortcuts[1].at("other") == "/location/to/shortcut/2", "Second shortcut should be '/location/to/shortcut/2'");
    }

    // Check logged messages
    //const auto& messages = logger->getMessages();
    //CHECK(!messages.empty());
    //CHECK(messages[0].first == "INFO");
    //CHECK(messages[0].second.find("Config loaded successfully") != std::string::npos);

    std::remove(configFile.c_str());
}

TEST_CASE("ConfigManager - Undo") {
    CAPTURE("Starting ConfigManager - Undo test");

    std::string configContent = R"(
# Keyboard shortcuts
remap:
  ctl+d: cmd+a
  ctl+2: shift+f1

# Plugins
rename-plugins:
  Serum: Daddy Duda's Special Synth

remove-plugins:
  - TerribleSynth
  - I'mTooLazyToUninstallSynth

# Quick shortcuts menu
shortcuts:
  - key: /location/to/shortcut/1
  - other: /location/to/shortcut/2

# Application behavior
init:
  retries: 3

window:
  search: 100,200,500,500
  preferences: 50,150,200,300
)";
    std::string configFile = "test_config.yaml";
    std::ofstream outFile(configFile);
    outFile << configContent;
    outFile.close();

    auto configManager = std::make_shared<ConfigManager>(configFile);
    configManager->loadConfig();

    CHECK_MESSAGE(configManager->getInitRetries() == 3, "retries should be 3");

    configManager->setInitRetries(5);
    CHECK_MESSAGE(configManager->getInitRetries() == 5, "retries set to 5");

    configManager->undo();
    CHECK_MESSAGE(configManager->getInitRetries() == 3, "retries after undo is 3");

    auto newManager = std::make_shared<ConfigManager>(configFile);
    newManager->loadConfig();
    CHECK(newManager->getInitRetries() == 3);

    std::remove(configFile.c_str());
}
