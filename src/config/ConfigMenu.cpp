#include <regex>
#include <algorithm>  // For std::find_if
#include <filesystem>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>

#include <QString>
#include <QFile>
#include <QTextStream>

#include "LogHandler.h"
#include "ConfigMenu.h"
#include "Types.h"

#include "yaml-cpp/yaml.h"

ConfigMenu::ConfigMenu(const std::filesystem::path& configFile)
    : configFile_(configFile) {
//    loadConfig();
    std::filesystem::path LESConfigFilePath =
        std::filesystem::path(std::string(getenv("HOME")))
        / ".les" / "menuconfig.ini"
    ;
    parseLESMenuConfig(LESConfigFilePath);

    LogHandler::getInstance().info("\n\n\n\n\n\n" + configFile.string());
}

void ConfigMenu::outputItemToYAML(YAML::Emitter& out, const MenuItem& item) {
    out << YAML::BeginMap;
    out << YAML::Key << "label" << YAML::Value << item.label;

    // Check if this item has children (a submenu)
    if (!item.children.empty()) {
        out << YAML::Key << "children" << YAML::BeginSeq;

        // Recursively output child items
        for (const auto& childItem : item.children) {
            outputItemToYAML(out, childItem);
        }

        out << YAML::EndSeq;
    } else {
        // If it's a regular item, output its action
        out << YAML::Key << "action" << YAML::Value << item.action;
    }

    out << YAML::EndMap;
}

void ConfigMenu::saveToYAML(const std::vector<MenuItem>& menuData, const std::filesystem::path& filePath) {
    YAML::Emitter out;
    out << YAML::BeginSeq;

    // Iterate over top-level menu items
    for (const auto& item : menuData) {
        outputItemToYAML(out, item);
    }

    out << YAML::EndSeq;

    // Write to file
    std::ofstream fout(filePath);
    fout << out.c_str();  // Write the YAML emitter output to file
    fout.close();
}

std::vector<MenuItem> ConfigMenu::getMenuData() {
    return menuData_;
}

void ConfigMenu::parseLESMenuConfig(const std::filesystem::path& filePath) {

    std::vector<MenuItem> menuData;  // Now a vector of MenuItem
    QString qFilePath = QString::fromStdString(filePath.string());

    QFile file(qFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // can't open file
        return;
    }

    QTextStream in(&file);
    MenuItem* currentItem = nullptr;  // Now we track current MenuItem
    std::vector<MenuItem*> categoryStack;  // Stack to keep track of nested submenus

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith(";")) {
            // Skip comments
            continue;
        } else if (line.startsWith("--")) {
            // Handle separators
            if (currentItem != nullptr) {
                MenuItem separator;
                separator.label = "--";
                separator.action = "separator";
                currentItem->children.push_back(separator);  // Add separator to current item
            }
        } else if (line.startsWith("..")) {
            // Go up one level in the category stack
            if (categoryStack.size() > 1) {
                categoryStack.pop_back();
                currentItem = categoryStack.back();
            } else {
                currentItem = nullptr;  // Go back to top level if stack is empty
            }
        } else if (line.startsWith("/")) {
            // Handle top-level categories or submenus
            std::string lineStr = line.toStdString();
            std::regex topLevelCategoryRegex("^/(?!nocategory)[^/](.*)");
            std::smatch matches;

            if (std::regex_search(lineStr, matches, topLevelCategoryRegex)) {
                if (matches.size() > 0) {
                    QString categoryName = QString::fromStdString(matches[0].str());

                    // Find or create a new top-level item
                    MenuItem newItem;
                    newItem.label = categoryName.toStdString();
                    newItem.action = "category";  // Category items can have no action

                    menuData.push_back(newItem);  // Add to top-level menu
                    currentItem = &menuData.back();
                    categoryStack.clear();
                    categoryStack.push_back(currentItem);
                }
            } else if (line.startsWith("/nocategory")) {
                // Reset to the top level
                currentItem = nullptr;
                categoryStack.clear();
            } else if (line.startsWith("//")) {
                // Handle subcategories
                QString subCategoryName = line.section('/', 2, 2);

                // Create a submenu item
                MenuItem subCategoryItem;
                subCategoryItem.label = subCategoryName.toStdString();
                subCategoryItem.action = "category";  // Subcategory items can also have no action

                currentItem->children.push_back(subCategoryItem);  // Add to current item
                currentItem = &currentItem->children.back();  // Set as the current item
                categoryStack.push_back(currentItem);
            }
        } else if (!line.isEmpty() && currentItem != nullptr) {
            // Regular menu items
            MenuItem item;
            item.label = line.toStdString();
            item.action = in.readLine().trimmed().toStdString();
            currentItem->children.push_back(item);  // Add item to current menu
        }
    }

    file.close();

    menuData_ = menuData;  // Update the class variable

    // Save to YAML as before
    std::filesystem::path configMenuFilePath =
        std::filesystem::path(std::string(getenv("HOME")))
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "config-menu.txt";
    saveToYAML(menuData, configMenuFilePath);
}

void ConfigMenu::loadConfig() {
    try {

    } catch (const std::exception &e) {
        LogHandler::getInstance().info("Error loading config: " + std::string(e.what()));
    }
}

