#include <filesystem>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "LogHandler.h"
#include "ConfigMenu.h"
#include "Types.h"
#include "yaml-cpp/yaml.h"

ConfigMenu::ConfigMenu(const std::filesystem::path& configFile)
    : configFile_(configFile) {
    std::filesystem::path LESConfigFilePath =
        std::filesystem::path(std::string(getenv("HOME"))) / ".les" / "menuconfig.ini";
    parseLESMenuConfig(LESConfigFilePath);
    LogHandler::getInstance().info("\n\n\n\n\n\n" + configFile.string());
}

void ConfigMenu::outputItemToYAML(YAML::Emitter& out, const MenuItem& item) {
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

void ConfigMenu::saveToYAML(const std::vector<MenuItem>& menuData, const std::filesystem::path& filePath) {
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

std::vector<MenuItem> ConfigMenu::getMenuData() {
    return menuData_;
}

void ConfigMenu::parseLESMenuConfig(const std::filesystem::path& filePath) {
    std::vector<MenuItem> menuData;
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LogHandler::getInstance().info("Unable to open file: " + filePath.string());
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

                    auto it = std::find_if(menuData.begin(), menuData.end(),
                                           [&categoryName](const MenuItem& item) {
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

                auto subIt = std::find_if(currentItem->children.begin(), currentItem->children.end(),
                                          [&subCategoryName](const MenuItem& subItem) {
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

    std::filesystem::path configMenuFilePath =
        std::filesystem::path(std::string(getenv("HOME"))) / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "config-menu.txt";
    saveToYAML(menuData, configMenuFilePath);
}

void ConfigMenu::loadConfig() {
    try {
        // Implementation
    } catch (const std::exception &e) {
        LogHandler::getInstance().info("Error loading config: " + std::string(e.what()));
    }
}
