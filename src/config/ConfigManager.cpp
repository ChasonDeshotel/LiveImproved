#include "LogHandler.h"
#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <numeric>

#include "Types.h"
#include "KeyMapper.h"

ConfigManager::ConfigManager(const std::filesystem::path& configFile)
    : configFile_(configFile) 
    , log_(&LogHandler::getInstance())
    , km_(new KeyMapper()) {
    loadConfig();

    log_->info("\n\n\n\n\n\n" + configFile.string());
}

void ConfigManager::loadConfig() {
    try {
        applyConfig(YAML::LoadFile(configFile_));
    } catch (const std::exception &e) {
        log_->info("Error loading config: " + std::string(e.what()));
    }
}

void ConfigManager::applyConfig(const YAML::Node& config) {
    try {
        undoStack_.push_back(YAML::Clone(config));

        if (config["init"]["retries"]) {
            initRetries_ = config["init"]["retries"].as<int>();
        }

        log_->info("remap before remap");
        if (config["remap"] && config["remap"].IsMap()) {
            for (const auto &item : config["remap"]) {
                log_->info("remap found remap");
                log_->info("remap: " + item.first.as<std::string>());
                log_->info("remap: " + item.second.as<std::string>());
                
                EKeyPress from = km_->processKeyPress(item.first.as<std::string>());
                std::string toField = item.second.as<std::string>();

                std::vector<std::string> toKeys;
                std::stringstream ss(toField);
                std::string key;
                while (std::getline(ss, key, ',')) {
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(' '));
                    key.erase(key.find_last_not_of(' ') + 1);
                    toKeys.push_back(key);
                }

                EKeyMacro macro;
                if (toKeys.size() == 1) {

                    macro.push_back(km_->processKeyPress(toKeys[0]));
                    remap_[from] = macro;
                } else {
                    for (const auto &keyStr : toKeys) {
                        macro.push_back(km_->processKeyPress(keyStr));
                    }
                    remap_[from] = macro;
                }
            }
        }

        if (config["rename-plugins"] && config["rename-plugins"].IsMap()) {
            for (const auto &item : config["rename-plugins"]) {
                std::string originalName = item.first.as<std::string>();
                std::string newName = item.second.as<std::string>();
                renamePlugins_[originalName] = newName;
            }
        } else {
            //throw std::runtime_error("'rename-plugins' section is missing or not a sequence");
        }

        if (config["remove-plugins"] && config["remove-plugins"].IsSequence()) {
            removePlugins_.clear();
            for (const auto& plugin : config["remove-plugins"]) {
                removePlugins_.push_back(plugin.as<std::string>());
            }
        } else {
            //throw std::runtime_error("'remove-plugins' section is missing or not a sequence");
        }

        if (config["window"] && config["window"].IsMap()) {
            for (const auto &item : config["window"]) {
                std::string windowName = item.first.as<std::string>();
                std::string setting = item.second.as<std::string>();
                windowSettings_[windowName] = setting;
            }
        } else {
            //throw std::runtime_error("'window' section is missing or not a sequence of maps");
        }

        if (config["shortcuts"] && config["shortcuts"].IsSequence()) {
            shortcuts_.clear();
            for (const auto& item : config["shortcuts"]) {
                std::unordered_map<std::string, std::string> shortcutMap;
                for (const auto& pair : item) {
                    std::string key = pair.first.as<std::string>();
                    std::string value = pair.second.as<std::string>();
                    shortcutMap[key] = value;
                }
                shortcuts_.push_back(shortcutMap);
            }
        } else {
            //throw std::runtime_error("'shortcuts' section is missing or not a sequence of maps");
        }

    } catch (const std::exception &e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
    }
}

void ConfigManager::saveConfig() {
    undoStack_.push_back(YAML::Clone(config_));

    config_["init"]["retries"] = initRetries_;

    YAML::Node remapNode = YAML::Load("{}");
    for (const auto& item : remap_) {
        log_->info("save remap");

        // Convert 'from' EKeyPress to string
        std::string fromString = km_->EKeyPressToString(item.first);
        log_->info("save remap from: " + fromString);

        // Convert 'to' (EKeyMacro) to string
        std::string toString;
        const EKeyMacro& macro = item.second;  // Assume item.second is of type EKeyMacro

        // Check if it's a single keypress or a macro with multiple keys
        if (macro.keypresses.size() == 1) {
            toString = km_->EKeyPressToString(macro.keypresses[0]);
            log_->info("save remap to (single): " + toString);
        } else {
            // If it's a macro, concatenate all keypresses
            std::vector<std::string> keyStrings;
            for (const auto& keyPress : macro.keypresses) {
                keyStrings.push_back(km_->EKeyPressToString(keyPress));
            }
            toString = std::accumulate(
                std::next(keyStrings.begin()), keyStrings.end(), keyStrings[0],
                [](std::string a, const std::string& b) {
                    return a + ", " + b;
                }
            );
            log_->info("save remap to (macro): " + toString);
        }

        remapNode[fromString] = toString;
    }

    YAML::Node renamePluginsNode = YAML::Load("{}");
    for (const auto &item : renamePlugins_) {
        renamePluginsNode[item.first] = item.second;
    }
    config_["rename-plugins"] = renamePluginsNode;

    YAML::Node removePluginsNode = YAML::Load("[]");
    for (const auto &plugin : removePlugins_) {
        removePluginsNode.push_back(plugin);
    }
    config_["remove-plugins"] = removePluginsNode;

    YAML::Node windowNode = YAML::Load("{}");
    for (const auto &item : windowSettings_) {
        windowNode[item.first] = item.second;
    }
    config_["window"] = windowNode;

    YAML::Node shortcutsNode = YAML::Load("[]");
    for (const auto &shortcut : shortcuts_) {
        shortcutsNode.push_back(shortcut);
    }
    config_["shortcuts"] = shortcutsNode;

    std::ofstream fout(configFile_);
    fout << config_;
}

int ConfigManager::getInitRetries() const {
    return initRetries_;
}

void ConfigManager::setInitRetries(int retries) {
    initRetries_ = retries;
    saveConfig();
}

std::unordered_map<EKeyPress, EKeyMacro, EKeyPressHash> ConfigManager::getRemap() const {
    log_->info("get remap caled");
    return remap_;
}

void ConfigManager::setRemap(const std::string &fromStr, const std::string &toStr) {
    EKeyPress from = km_->processKeyPress(fromStr);
    EKeyPress to   = km_->processKeyPress(toStr);

    EKeyMacro macro;
    macro.push_back(to);

    log_->info("set remap");
    log_->info("set remap from: " + from.key);
    log_->info("set remap to: " + to.key);
    remap_[from] = macro;
    saveConfig();
}

std::unordered_map<std::string, std::string> ConfigManager::getRenamePlugins() const {
    return renamePlugins_;
}

void ConfigManager::setRenamePlugin(const std::string &originalName, const std::string &newName) {
    renamePlugins_[originalName] = newName;
    saveConfig();
}

std::vector<std::string> ConfigManager::getRemovePlugins() const {
    return removePlugins_;
}

void ConfigManager::setRemovePlugin(const std::string &pluginName) {
    removePlugins_.push_back(pluginName);
    saveConfig();
}

std::unordered_map<std::string, std::string> ConfigManager::getWindowSettings() const {
    return windowSettings_;
}

void ConfigManager::setWindowSetting(const std::string &windowName, const std::string &setting) {
    windowSettings_[windowName] = setting;
    saveConfig();
}

std::vector<std::unordered_map<std::string, std::string>> ConfigManager::getShortcuts() const {
    return shortcuts_;
}

void ConfigManager::setShortcut(size_t index, const std::unordered_map<std::string, std::string>& shortcut) {
    if (index < shortcuts_.size()) {
        shortcuts_[index] = shortcut;
    } else {
        throw std::out_of_range("Index is out of range");
    }

    saveConfig();
}

void ConfigManager::undo() {
    if (canUndo()) {
        // Remove the current state
        undoStack_.pop_back();

        if (canUndo()) {
            applyConfig(YAML::Clone(undoStack_.back()));
            saveConfig();
        } else {
            std::cerr << "No more states to undo." << std::endl;
        }
    } else {
        std::cerr << "Undo stack is empty, cannot undo." << std::endl;
    }
}


bool ConfigManager::canUndo() const {
    return !undoStack_.empty();
}

