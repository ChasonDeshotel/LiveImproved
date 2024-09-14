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

    log_->info("config filepath:" + configFile.string());
}

void ConfigManager::loadConfig() {
    try {
        applyConfig(YAML::LoadFile(configFile_));
    } catch (const std::exception &e) {
        log_->error("Error loading config: " + std::string(e.what()));
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
                log_->debug("remap found remap");
                log_->debug("remap fromStr: " + item.first.as<std::string>());
                log_->debug("remap toStr: "   + item.second.as<std::string>());
            
                processRemap(item.first.as<std::string>(), item.second.as<std::string>());
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
        log_->debug("save remap");

        std::string fromString = km_->EKeyPressToString(item.first);
        log_->debug("save remap from: " + fromString);

        std::vector<std::string> stepStrings;
        const EMacro& macro = item.second;

        for (const auto& item : macro.steps) {
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, EKeyPress>) {
                    stepStrings.push_back(km_->EKeyPressToString(arg));
                } else if constexpr (std::is_same_v<T, Action>) {
                    if (arg.arguments) {
                        stepStrings.push_back(arg.actionName + "." + *arg.arguments);
                    } else {
                        stepStrings.push_back(arg.actionName);
                    }
                }
            }, item);
        }

        std::string toString;
        toString = std::accumulate(
            std::next(stepStrings.begin()), stepStrings.end(), stepStrings[0],
            [](std::string a, const std::string& b) {
                return a + ", " + b;
            }
        );

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

std::unordered_map<EKeyPress, EMacro, EMacroHash> ConfigManager::getRemap() const {
    log_->debug("get remap caled");
    return remap_;
}

void ConfigManager::setRemap(const std::string &fromStr, const std::string &toStr) {
    log_->debug("setRemap: from: " + fromStr + " to: " + toStr);
    processRemap(fromStr, toStr);
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

void ConfigManager::processRemap(const std::string &fromStr, const std::string &toStr) {
    log_->debug("process remap: from: " + fromStr + " to: " + toStr);
    EKeyPress from = km_->processKeyPress(fromStr);

    // stepsString is composed of any combination of
    // actions and/or keypresses. For example,
    // "cmd+a" or "load_item.Serum" or "a,load_item.Serum,shift+cmd+d,c"
    std::vector<std::string> steps;
    std::stringstream ss(toStr);
    std::string stepsString;
    while (std::getline(ss, stepsString, ',')) {
        stepsString.erase(0, stepsString.find_first_not_of(' '));
        stepsString.erase(stepsString.find_last_not_of(' ') + 1);
        steps.push_back(stepsString);
    }

    EMacro macro;

    for (const auto& step : steps) {
        log_->debug("process remap: step: " + step);

        const auto& namedActions = NamedActions::get();

        if (step.length() == 1) {
            log_->debug("process remap: single character, processing as keypress: " + step);
            EKeyPress keyPress = km_->processKeyPress(step);
            macro.addKeyPress(keyPress);

        } else if (namedActions.find(step) != namedActions.end()) {
            log_->debug("process remap: add action no arg: " + step);
            Action action(step);
            macro.addAction(action);

        } else if (step.find('.') != std::string::npos) {
            // split on . to separate action name and argument
            size_t periodPos = step.find('.');
            std::string actionName = step.substr(0, periodPos);
            std::optional<std::string> argument = step.substr(periodPos + 1);

            if (namedActions.find(actionName) != namedActions.end()) {
                if (argument) {
                    log_->debug("process remap: add action with arg: " + actionName + "." + *argument);
                    Action action(actionName, *argument);
                    macro.addAction(action);
                }
            } else {
                // step contains a period but no valid action
                log_->warn("step contains . but no valid action. action given: " + actionName);
                continue;
            }

        } else {
            // TODO consider turning these into a keypress macro
            log_->debug("process remap: not in named actions. str: " + step);
            EKeyPress keyPress = km_->processKeyPress(step);
            macro.addKeyPress(keyPress);
        }
    }

    remap_[from] = macro;
}
