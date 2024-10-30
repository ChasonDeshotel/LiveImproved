#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ConfigManager.h"
#include "DependencyContainer.h"
#include "MockLogHandler.h"
#include "KeyMapper.h"

#include <fstream>
#include <filesystem>

class ConfigManagerTestFixture {
public:
    ConfigManagerTestFixture() {
        container = std::make_unique<DependencyContainer>();
        logger = std::make_shared<MockLogHandler>();
        container->registerFactory<ILogHandler>(
            [this](DependencyContainer&) { return logger; },
            DependencyContainer::Lifetime::Singleton
        );
        container->registerFactory<KeyMapper>(
            [](DependencyContainer& c) { return std::make_shared<KeyMapper>(); },
            DependencyContainer::Lifetime::Singleton
        );
    }

    ~ConfigManagerTestFixture() {
        if (std::filesystem::exists(configFile)) {
            std::filesystem::remove(configFile);
        }
    }

    void createConfigFile(const std::string& content) {
        std::ofstream outFile(configFile);
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to create temp file: " + configFile);
        }
        outFile << content;
        outFile.close();
    }

    std::unique_ptr<DependencyContainer> container;
    std::shared_ptr<MockLogHandler> logger;
    std::string configFile = "test_config.yaml";
};

TEST_CASE_FIXTURE(ConfigManagerTestFixture, "ConfigManager - Load Config") {
    std::string configContent = R"(
remap:
  ctrl+d: cmd+a
  d: delete
  alt+a: plugin.Serum

rename-plugins:
  Serum: Daddy Duda's Special Synth

remove-plugins:
  - TerribleSynth
  - I'mTooLazyToUninstallSynth

shortcuts:
  - key: /location/to/shortcut/1
  - other: /location/to/shortcut/2

init:
  retries: 10

window:
  search: 100,200,500,500
  preferences: 50,150,200,300
)";
    createConfigFile(configContent);

    auto configManager = std::make_shared<ConfigManager>(configFile);
    configManager->loadConfig();

    SUBCASE("Check init retries") {
        CHECK(configManager->getInitRetries() == 10);
    }

    SUBCASE("Check remap") {
        auto remap = configManager->getRemap();
        CHECK(remap.size() == 3);

        auto km = container->resolve<KeyMapper>();
        
        EKeyPress ctrlD = km->processKeyPress("ctrl+d");
        CHECK(remap.find(ctrlD) != remap.end());
        CHECK(remap[ctrlD].steps.size() == 1);
        CHECK(std::holds_alternative<EKeyPress>(remap[ctrlD].steps[0]));
        CHECK(std::get<EKeyPress>(remap[ctrlD].steps[0]) == km->processKeyPress("cmd+a"));

        EKeyPress d = km->processKeyPress("d");
        CHECK(remap.find(d) != remap.end());
        CHECK(remap[d].steps.size() == 1);
        CHECK(std::holds_alternative<EKeyPress>(remap[d].steps[0]));
        CHECK(std::get<EKeyPress>(remap[d].steps[0]) == km->processKeyPress("delete"));

        EKeyPress altA = km->processKeyPress("alt+a");
        CHECK(remap.find(altA) != remap.end());
        CHECK(remap[altA].steps.size() == 1);
        CHECK(std::holds_alternative<Action>(remap[altA].steps[0]));
        CHECK(std::get<Action>(remap[altA].steps[0]).actionName == "plugin");
        CHECK(std::get<Action>(remap[altA].steps[0]).arguments.value() == "Serum");
    }

    SUBCASE("Check rename plugins") {
        auto renamePlugins = configManager->getRenamePlugins();
        CHECK(renamePlugins.size() == 1);
        CHECK(renamePlugins["Serum"] == "Daddy Duda's Special Synth");
    }

    SUBCASE("Check remove plugins") {
        auto removePlugins = configManager->getRemovePlugins();
        CHECK(removePlugins.size() == 2);
        CHECK(removePlugins[0] == "TerribleSynth");
        CHECK(removePlugins[1] == "I'mTooLazyToUninstallSynth");
    }

    SUBCASE("Check window settings") {
        auto windowSettings = configManager->getWindowSettings();
        CHECK(windowSettings.size() == 2);
        CHECK(windowSettings["search"] == "100,200,500,500");
        CHECK(windowSettings["preferences"] == "50,150,200,300");
    }

    SUBCASE("Check shortcuts") {
        auto shortcuts = configManager->getShortcuts();
        CHECK(shortcuts.size() == 2);
        CHECK(shortcuts[0].at("key") == "/location/to/shortcut/1");
        CHECK(shortcuts[1].at("other") == "/location/to/shortcut/2");
    }
}

TEST_CASE_FIXTURE(ConfigManagerTestFixture, "ConfigManager - Save Config") {
    std::string configContent = R"(
init:
  retries: 3
)";
    createConfigFile(configContent);

    auto configManager = std::make_shared<ConfigManager>(configFile);
    configManager->loadConfig();

    SUBCASE("Save and reload config") {
        configManager->setInitRetries(5);
        configManager->setRemap("ctrl+s", "cmd+s");
        configManager->setRenamePlugin("OldPlugin", "NewPlugin");
        configManager->setRemovePlugin("BadPlugin");
        configManager->setWindowSetting("main", "100,100,800,600");
        configManager->setShortcut(0, {{"newKey", "/new/shortcut"}});

        configManager->saveConfig();

        // Create a new ConfigManager instance to load the saved config
        auto newConfigManager = std::make_shared<ConfigManager>(configFile);
        newConfigManager->loadConfig();

        CHECK(newConfigManager->getInitRetries() == 5);
        
        auto remap = newConfigManager->getRemap();
        auto km = container->resolve<KeyMapper>();
        EKeyPress ctrlS = km->processKeyPress("ctrl+s");
        CHECK(remap.find(ctrlS) != remap.end());
        CHECK(remap[ctrlS].steps.size() == 1);
        CHECK(std::holds_alternative<EKeyPress>(remap[ctrlS].steps[0]));
        CHECK(std::get<EKeyPress>(remap[ctrlS].steps[0]) == km->processKeyPress("cmd+s"));

        auto renamePlugins = newConfigManager->getRenamePlugins();
        CHECK(renamePlugins["OldPlugin"] == "NewPlugin");

        auto removePlugins = newConfigManager->getRemovePlugins();
        CHECK(std::find(removePlugins.begin(), removePlugins.end(), "BadPlugin") != removePlugins.end());

        auto windowSettings = newConfigManager->getWindowSettings();
        CHECK(windowSettings["main"] == "100,100,800,600");

        auto shortcuts = newConfigManager->getShortcuts();
        CHECK(shortcuts.size() == 1);
        CHECK(shortcuts[0].at("newKey") == "/new/shortcut");
    }
}

TEST_CASE_FIXTURE(ConfigManagerTestFixture, "ConfigManager - Undo") {
    std::string configContent = R"(
init:
  retries: 3
)";
    createConfigFile(configContent);

    auto configManager = std::make_shared<ConfigManager>(configFile);
    configManager->loadConfig();

    SUBCASE("Undo changes") {
        CHECK(configManager->getInitRetries() == 3);

        configManager->setInitRetries(5);
        CHECK(configManager->getInitRetries() == 5);

        configManager->undo();
        CHECK(configManager->getInitRetries() == 3);

        // Check that we can't undo past the initial state
        configManager->undo();
        CHECK(configManager->getInitRetries() == 3);
    }
}
