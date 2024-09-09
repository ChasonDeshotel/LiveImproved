#ifndef CONFIG_MENU_H
#define CONFIG_MENU_H

#include "yaml-cpp/yaml.h"
#include "Types.h"

class ConfigMenu {
public:
    explicit ConfigMenu(const std::filesystem::path& configFile);

    void loadConfig();
    void saveConfig();

    void undo();
    bool canUndo() const;

    void parseLESMenuConfig(const std::filesystem::path& filePath);

    std::vector<MenuCategory> getMenuData();

private:
    std::vector<MenuCategory> menuData_;

    void saveToYAML(const std::vector<MenuCategory>& menuData, const std::filesystem::path& filePath);
    void saveCategoryToYAML(YAML::Emitter& out, const MenuCategory& category);
    void applyConfig(const YAML::Node& config);

    std::string configFile_;

    YAML::Node config_;

    std::vector<YAML::Node> undoStack_;

};

#endif
