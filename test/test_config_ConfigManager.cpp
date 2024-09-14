#define BOOST_TEST_MODULE ConfigManagerTest
#include <boost/test/included/unit_test.hpp>
#include "ConfigManager.h"
#include <fstream>

#include "Types.h"

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

BOOST_AUTO_TEST_CASE(test_loadConfig) {
    std::string configContent = R"(
# Keyboard shortcuts
remap:
  ctl+d: cmd+a
  ctl+2: shift+f
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
    std::string configFile = createTempConfigFile(configContent);

    ConfigManager configManager(configFile);

    BOOST_CHECK_EQUAL(configManager.getInitRetries(), 10);

    auto remap = configManager.getRemap();

//    BOOST_CHECK_EQUAL();

//    BOOST_CHECK_EQUAL(remap[kpFrom], marco);
//
//    EKeyPress kp3From  ; EKeyPress kp3To   ;
//    kpFrom.key = "d" ; kpTo.key = "delete" ;
//    EMacro mario ; mario.addKeyPress(kp3To);
//
//    BOOST_CHECK_EQUAL(remap[kpFrom], mario);
//
//    Action acTo(   "plugin"  ,  "Serum"   );
//    EKeyPress acKFrom  ; EKeyPress acKTo   ;
//    acKFrom.alt = true ; acKFrom.key = "d" ;
//
//    BOOST_CHECK_EQUAL(remap[kpFrom], acTo) ;
//
//    BOOST_CHECK_EQUAL(acTo.actionName, "plugin");
//    BOOST_CHECK_EQUAL(acTo.arguments , "Serum" );
//
//    auto renamePlugins = configManager.getRenamePlugins();
//    BOOST_CHECK_EQUAL(renamePlugins["Serum"], "Daddy Duda's Special Synth");
//
//    auto removePlugins = configManager.getRemovePlugins();
//    if (removePlugins.empty()) {
//        throw std::runtime_error("removePlugins vector is empty or not loaded correctly.");
//    }
//    BOOST_CHECK_EQUAL(removePlugins[0], "TerribleSynth");
//    BOOST_CHECK_EQUAL(removePlugins[1], "I'mTooLazyToUninstallSynth");
//
//    auto windowSettings = configManager.getWindowSettings();
//    BOOST_CHECK_EQUAL(windowSettings["search"], "100,200,500,500");
//    BOOST_CHECK_EQUAL(windowSettings["preferences"], "50,150,200,300");
//
//    auto shortcuts = configManager.getShortcuts();
//
//    BOOST_REQUIRE_EQUAL(shortcuts.size(), 2);
//    BOOST_CHECK_EQUAL(shortcuts[0].at("key"), "/location/to/shortcut/1");
//    BOOST_CHECK_EQUAL(shortcuts[1].at("other"), "/location/to/shortcut/2");
//
//    deleteTempConfigFile(configFile);
}

BOOST_AUTO_TEST_CASE(test_setInitRetries) {
    std::string configContent = R"(
# Keyboard shortcuts
remap:
  ctl+d: cmd+a
  ctl+2: shift+f1

# Plugins
rename-plugins:
  Serum: Daddy Duda's Special Synthtest/test_config_ConfigManager.cpp

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
    std::string configFile = createTempConfigFile(configContent);

    ConfigManager configManager(configFile);

    BOOST_CHECK_EQUAL(configManager.getInitRetries(), 3);
    
    configManager.setInitRetries(5);

    BOOST_CHECK_EQUAL(configManager.getInitRetries(), 5);

    ConfigManager newManager(configFile);
    BOOST_CHECK_EQUAL(newManager.getInitRetries(), 5);

    deleteTempConfigFile(configFile);
}
//
//BOOST_AUTO_TEST_CASE(test_undo) {
//    std::string configContent = R"(
//# Keyboard shortcuts
//remap:
//  ctl+d: cmd+a
//  ctl+2: shift+f1
//
//# Plugins
//rename-plugins:
//  Serum: Daddy Duda's Special Synth
//
//remove-plugins:
//  - TerribleSynth
//  - I'mTooLazyToUninstallSynth
//
//# Quick shortcuts menu
//shortcuts:
//  - key: /location/to/shortcut/1
//  - other: /location/to/shortcut/2
//
//# Application behavior
//init:
//  retries: 3
//
//window:
//  search: 100,200,500,500
//  preferences: 50,150,200,300
//)";
//    std::string configFile = createTempConfigFile(configContent);
//
//    ConfigManager configManager(configFile);
//
//    BOOST_CHECK_EQUAL(configManager.getInitRetries(), 3);
//
//    configManager.setInitRetries(5);
//    BOOST_CHECK_EQUAL(configManager.getInitRetries(), 5);
//
//    configManager.undo();
//    BOOST_CHECK_EQUAL(configManager.getInitRetries(), 3);
//
//    ConfigManager newManager(configFile);
//    BOOST_CHECK_EQUAL(newManager.getInitRetries(), 3);
//
//    deleteTempConfigFile(configFile);
//}
