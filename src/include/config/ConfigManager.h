#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <cstdlib>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include "yaml-cpp/yaml.h"

#include "Types.h"

class KeyMapper;

class ConfigManager {
public:
    explicit ConfigManager(std::filesystem::path configFile);

    void loadConfig();
    void saveConfig();

    auto getInitRetries() const -> int;
    void setInitRetries(int retries);

    auto getRemap() const -> std::unordered_map<EKeyPress, EMacro, EMacroHash>;
    void setRemap(const std::string &from, const std::string &to);

    auto getRenamePlugins() const -> std::unordered_map<std::string, std::string>;
    void setRenamePlugin(const std::string &originalName, const std::string &newName);

    auto getRemovePlugins() const -> std::vector<std::string>;
    void setRemovePlugin(const std::string &pluginName);

    auto getWindowSettings() const -> std::unordered_map<std::string, std::string>;
    void setWindowSetting(const std::string &windowName, const std::string &setting);

    auto getShortcuts() const -> std::vector<std::unordered_map<std::string, std::string>>;
    void setShortcut(size_t index, const std::unordered_map<std::string, std::string> &shortcut);

    void undo();
    auto canUndo() const -> bool;

private:
    KeyMapper* km_;

    void applyConfig(const YAML::Node& config);

    void processRemap(const std::string &from, const std::string &to);

    std::filesystem::path configFile_;
    YAML::Node config_;

    // Configuration options
    int initRetries_;
    std::unordered_map<EKeyPress, EMacro, EMacroHash> remap_;
    std::unordered_map<std::string, std::string> renamePlugins_;
    std::vector<std::string> removePlugins_;
    std::unordered_map<std::string, std::string> windowSettings_;
    std::vector<std::unordered_map<std::string, std::string>> shortcuts_;

    std::vector<YAML::Node> undoStack_;
};

#endif // CONFIG_MANAGER_H
