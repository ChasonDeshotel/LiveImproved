#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>

#include "LogGlobal.h"
#include "Types.h"

#include "ConfigManager.h"
#include "KeyMapper.h"

ConfigManager::ConfigManager(std::filesystem::path configFile)
    : configFile_(std::move(configFile))
    , km_(new KeyMapper())
    , initRetries_() {
    loadConfig();
}

auto ConfigManager::loadConfig() -> void {
    try {
        if (!std::filesystem::exists(configFile_)) {
            throw std::runtime_error("Config file does not exist: " + configFile_.string());
        }

        if (!std::filesystem::is_regular_file(configFile_)) {
            throw std::runtime_error("Config file is not a regular file: " + configFile_.string());
        }

        YAML::Node config = YAML::LoadFile(configFile_.generic_string());
        logger->info("Config file loaded successfully");

        applyConfig(config);

    } catch (const std::exception &e) {
        logger->error("Error parsing config: " + std::string(e.what()));
    }
}

auto ConfigManager::applyConfig(const YAML::Node& config) -> void {
    try {
        undoStack_.push_back(YAML::Clone(config));

        if (config["init"]["retries"]) {
            initRetries_ = config["init"]["retries"].as<int>();
        }

        //logger->info("remap before remap");
        if (config["remap"] && config["remap"].IsMap()) {
            for (const auto &item : config["remap"]) {
                //logger->debug("remap found remap");
                //logger->debug("remap fromStr: " + item.first.as<std::string>());
                //logger->debug("remap toStr: "   + item.second.as<std::string>());

                processRemap(item.first.as<std::string>(), item.second.as<std::string>());
            }
        }

        if (config["rename-plugins"] && config["rename-plugins"].IsMap()) {
            for (const auto &item : config["rename-plugins"]) {
                auto originalName = item.first.as<std::string>();
                auto newName = item.second.as<std::string>();
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
                auto windowName = item.first.as<std::string>();
                auto setting = item.second.as<std::string>();
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
                    auto key = pair.first.as<std::string>();
                    auto value = pair.second.as<std::string>();
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


auto ConfigManager::saveConfig() -> void {
    undoStack_.push_back(YAML::Clone(config_));

    config_["init"]["retries"] = initRetries_;

    YAML::Node remapNode = YAML::Load("{}");
    for (const auto& item : remap_) {
        //logger->debug("save remap");

        std::string fromString = km_->EKeyPressToString(item.first);
        //logger->debug("save remap from: " + fromString);

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

auto ConfigManager::getInitRetries() const -> int {
    return initRetries_;
}

auto ConfigManager::setInitRetries(int retries) -> void {
    initRetries_ = retries;
    saveConfig();
}

auto ConfigManager::getRemap() const -> std::unordered_map<EKeyPress, EMacro, EMacroHash> {
    //logger->debug("get remap caled");
    return remap_;
}

auto ConfigManager::setRemap(const std::string &fromStr, const std::string &toStr) -> void {
    //logger->debug("setRemap: from: " + fromStr + " to: " + toStr);
    processRemap(fromStr, toStr);
    saveConfig();
}

auto ConfigManager::getRenamePlugins() const -> std::unordered_map<std::string, std::string> {
    return renamePlugins_;
}

auto ConfigManager::setRenamePlugin(const std::string &originalName, const std::string &newName) -> void {
    renamePlugins_[originalName] = newName;
    saveConfig();
}

auto ConfigManager::getRemovePlugins() const -> std::vector<std::string> {
    return removePlugins_;
}

auto ConfigManager::setRemovePlugin(const std::string &pluginName) -> void {
    removePlugins_.push_back(pluginName);
    saveConfig();
}

auto ConfigManager::getWindowSettings() const -> std::unordered_map<std::string, std::string> {
    return windowSettings_;
}

auto ConfigManager::setWindowSetting(const std::string &windowName, const std::string &setting) -> void {
    windowSettings_[windowName] = setting;
    saveConfig();
}

auto ConfigManager::getShortcuts() const -> std::vector<std::unordered_map<std::string, std::string>> {
    return shortcuts_;
}

auto ConfigManager::setShortcut(size_t index, const std::unordered_map<std::string, std::string>& shortcut) -> void {
    if (index < shortcuts_.size()) {
        shortcuts_[index] = shortcut;
    } else {
        throw std::out_of_range("Index is out of range");
    }

    saveConfig();
}

auto ConfigManager::undo() -> void {
    if (canUndo()) {
        // Remove the current state
        undoStack_.pop_back();

        if (canUndo()) {
            applyConfig(YAML::Clone(undoStack_.back()));
            saveConfig();
        } else {
            logger->warn("No more states to undo.");
        }
    } else {
        logger->warn("Undo stack is empty, cannot undo.");
    }
}

auto ConfigManager::canUndo() const -> bool {
    return !undoStack_.empty();
}

auto ConfigManager::processRemap(const std::string &fromStr, const std::string &toStr) -> void {
    //logger->debug("process remap: from: " + fromStr + " to: " + toStr);
    EKeyPress from = km_->processKeyPress(fromStr);

    // stepsString is composed of any combination of
    // actions and/or keypresses. For example,
    // "cmd+a" or "load_item.Serum" or "a,load_item.Serum,shift+cmd+d,c"
    std::vector<std::string> steps;
    std::stringstream ss(toStr);
    std::string stepsString;
    while (std::getline(ss, stepsString, ',')) {
        stepsString.erase(0, stepsString.find_first_not_of(" \t\n\r"));
        stepsString.erase(stepsString.find_last_not_of(" \t\n\r") + 1);
        steps.push_back(stepsString);
    }

    EMacro macro;

    for (const auto& step : steps) {
        //logger->debug("process remap: step: " + step);

        const auto& namedActions = NamedActions::get();

        if (step.length() == 1) {
            //logger->debug("process remap: single character, processing as keypress: " + step);
            try {
                EKeyPress keyPress = km_->processKeyPress(step);
                macro.addKeyPress(keyPress);
            } catch (const std::runtime_error& e) {
                logger->error(e.what());
            }

        } else if (namedActions.find(step) != namedActions.end()) {
            //logger->debug("process remap: add action no arg: " + step);
            Action action(step);
            // TODO add validation
            macro.addAction(action);

        } else if (step.find('.') != std::string::npos) {
            // split on . to separate action name and argument
            size_t periodPos = step.find('.');
            std::string actionName = step.substr(0, periodPos);
            std::optional<std::string> argument = step.substr(periodPos + 1);

            if (namedActions.find(actionName) != namedActions.end()) {
                if (argument) {
                    //logger->debug("process remap: add action with arg: " + actionName + "." + *argument);
                    Action action(actionName, *argument);
                    // TODO add validation
                    macro.addAction(action);
                }
            } else {
                // step contains a period but no valid action
                logger->warn("step contains . but no valid action. action given: " + actionName);
                continue;
            }

        } else {
            // TODO consider turning these into a keypress macro
            //logger->debug("process remap: not in named actions. str: " + step);
            try {
                EKeyPress keyPress = km_->processKeyPress(step);
                macro.addKeyPress(keyPress);
            } catch (const std::runtime_error& e) {
                logger->error(e.what());
            }
        }
    }

    remap_[from] = macro;
}
