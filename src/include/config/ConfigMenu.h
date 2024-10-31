#pragma once

#include <filesystem>
#include <yaml-cpp/yaml.h>

class MenuItem;

class ConfigMenu {
public:
    explicit ConfigMenu(const std::filesystem::path& configFile);

    void loadConfig();
    void saveConfig();

    void undo();
    auto canUndo() const -> bool;

    void parseLESMenuConfig(const std::filesystem::path& filePath);

    auto getMenuData() -> std::vector<MenuItem>;

private:
    std::vector<MenuItem> menuData_;

    void saveToYAML(const std::vector<MenuItem>& menuData, const std::filesystem::path& filePath);
    void outputItemToYAML(YAML::Emitter& out, const MenuItem& item);
    void applyConfig(const YAML::Node& config);

    std::filesystem::path configFile_;

    YAML::Node config_;

    std::vector<YAML::Node> undoStack_;

    void applyConfig(const YAML::Node& config);
    MenuItem parseMenuItem(const YAML::Node& node);
};
