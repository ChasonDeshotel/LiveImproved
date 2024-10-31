#include <filesystem>
#include <regex>
#include <algorithm>
#include <ranges>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "yaml-cpp/yaml.h"

#include "PathManager.h"

#include "LogGlobal.h"

#include "ConfigMenu.h"
#include "IWindow.h"

ConfigMenu::ConfigMenu(std::filesystem::path configFile)
    : configFile_(std::move(configFile)) {
    parseLESMenuConfig(PathManager().lesConfig());
}

auto ConfigMenu::outputItemToYAML(YAML::Emitter& out, const MenuItem& item) -> void {
    out << YAML::BeginMap;
    out << YAML::Key << "label" << YAML::Value << item.label;

    if (!item.children.empty()) {
        out << YAML::Key << "children" << YAML::BeginSeq;
        for (const auto& childItem : item.children) {
            outputItemToYAML(out, childItem);
        }
        out << YAML::EndSeq;
    } else {
        out << YAML::Key << "action" << YAML::Value << item.action;
    }

    out << YAML::EndMap;
}

auto ConfigMenu::saveToYAML(const std::vector<MenuItem>& menuData, const std::filesystem::path& filePath) -> void {
    YAML::Emitter out;
    out << YAML::BeginSeq;

    for (const auto& item : menuData) {
        outputItemToYAML(out, item);
    }

    out << YAML::EndSeq;

    std::ofstream fout(filePath);
    fout << out.c_str();
    fout.close();
}

auto ConfigMenu::getMenuData() -> std::vector<MenuItem> {
    return menuData_;
}

auto ConfigMenu::parseLESMenuConfig(const std::filesystem::path& filePath) -> void {
    std::vector<MenuItem> menuData;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        logger->error("Unable to open file: " + filePath.string());
        return;
    }

    MenuItem* currentItem = nullptr;
    std::vector<MenuItem*> categoryStack;
    std::string line;

    while (std::getline(file, line)) {
        line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), ""); // Trim line

        if (line.empty() || line[0] == ';') {
            continue;
        } else if (line.starts_with("--")) {
            if (currentItem) {
                MenuItem separator;
                separator.label = "--";
                separator.action = "separator";
                currentItem->children.push_back(separator);
            }
        } else if (line == "..") {
            if (categoryStack.size() > 1) {
                categoryStack.pop_back();
                currentItem = categoryStack.back();
            } else {
                currentItem = nullptr;
            }
        } else if (line.starts_with("/")) {
            std::regex topLevelCategoryRegex("^/(?!nocategory)[^/](.*)");
            std::smatch matches;

            if (std::regex_search(line, matches, topLevelCategoryRegex)) {
                if (!matches.empty()) {
                    std::string categoryName = matches[0].str().substr(1);  // Remove the first "/"

                    auto it = std::ranges::find_if(menuData, [&categoryName](const MenuItem& item) {
                        return item.label == categoryName;
                    });

                    if (it != menuData.end()) {
                        currentItem = &(*it);
                    } else {
                        MenuItem newItem;
                        newItem.label = categoryName;
                        newItem.action = "category";
                        menuData.push_back(newItem);
                        currentItem = &menuData.back();
                    }

                    categoryStack.clear();
                    categoryStack.push_back(currentItem);
                }
            } else if (line.starts_with("/nocategory")) {
                currentItem = nullptr;
                categoryStack.clear();
            } else if (line.starts_with("//")) {
                std::string subCategoryName = line.substr(2);  // Remove leading "//"

                auto subIt = std::ranges::find_if(currentItem->children, [&subCategoryName](const MenuItem& subItem) {
                    return subItem.label == subCategoryName;
                });

                if (subIt != currentItem->children.end()) {
                    currentItem = &(*subIt);
                } else {
                    MenuItem subCategoryItem;
                    subCategoryItem.label = subCategoryName;
                    subCategoryItem.action = "category";
                    currentItem->children.push_back(subCategoryItem);
                    currentItem = &currentItem->children.back();
                }

                categoryStack.push_back(currentItem);
            }
        } else if (currentItem) {
            MenuItem item;
            item.label = line;
            if (std::getline(file, line)) {
                item.action = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");  // Trimmed action
            }
            currentItem->children.push_back(item);
        }
    }

    file.close();
    menuData_ = menuData;

    auto configMenuFilePath = PathManager().configMenu();

    saveToYAML(menuData, configMenuFilePath);
}

auto ConfigMenu::loadConfig() -> void {
    try {
        // Implementation
    } catch (const std::exception &e) {
        logger->error("Error loading config: " + std::string(e.what()));
    }
}
