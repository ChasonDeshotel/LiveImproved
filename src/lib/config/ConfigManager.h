#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include "yaml-cpp/yaml.h"

class ConfigManager {
public:
    explicit ConfigManager(const std::filesystem::path& configFile);

    void loadConfig();
    void saveConfig();

    int getInitRetries() const;
    void setInitRetries(int retries);

    std::unordered_map<std::string, std::string> getRemap() const;
    void setRemap(const std::string &from, const std::string &to);

    std::unordered_map<std::string, std::string> getRenamePlugins() const;
    void setRenamePlugin(const std::string &originalName, const std::string &newName);

    std::vector<std::string> getRemovePlugins() const;
    void setRemovePlugin(const std::string &pluginName);

    std::unordered_map<std::string, std::string> getWindowSettings() const;
    void setWindowSetting(const std::string &windowName, const std::string &setting);

    std::vector<std::unordered_map<std::string, std::string>> getShortcuts() const;
    void setShortcut(size_t index, const std::unordered_map<std::string, std::string> &shortcut);

    void undo();
    bool canUndo() const;

private:
    void applyConfig(const YAML::Node& config);

    std::string configFile_;
    YAML::Node config_;

    // Configuration options
    int initRetries_;
    std::unordered_map<std::string, std::string> remap_;
    std::unordered_map<std::string, std::string> renamePlugins_;
    std::vector<std::string> removePlugins_;
    std::unordered_map<std::string, std::string> windowSettings_;
    std::vector<std::unordered_map<std::string, std::string>> shortcuts_;

    std::vector<YAML::Node> undoStack_;
};

#endif // CONFIG_MANAGER_H
