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
#include "Menu.h"

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
void outputItemToYAML(YAML::Emitter& out, const MenuItem& item) {
    out << YAML::BeginMap;
    out << YAML::Key << "label" << YAML::Value << item.label;

    // Check if this item has a subcategory
    if (item.subCategory) {
        out << YAML::Key << "subcategory" << YAML::BeginMap;
        out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;

        // Recursively output subcategory items
        for (const auto& subItem : item.subCategory->items) {
            outputItemToYAML(out, subItem);
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;
    } else {
        // If it's a regular item, output its action
        out << YAML::Key << "action" << YAML::Value << item.action;
    }

    out << YAML::EndMap;
}
void outputCategoryToYAML(YAML::Emitter& out, const MenuCategory& category) {
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << category.name;
    out << YAML::Key << "items" << YAML::Value << YAML::BeginSeq;

    for (const auto& item : category.items) {
        outputItemToYAML(out, item);
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;
}

// Helper function to find an existing category by name
// Main function to output all categories to YAML
void ConfigMenu::saveToYAML(const std::vector<MenuCategory>& menuData, const std::filesystem::path& filePath) {
    YAML::Emitter out;
    out << YAML::BeginSeq;

    for (const auto& category : menuData) {
        outputCategoryToYAML(out, category);
    }

    out << YAML::EndSeq;

    // Write to file
    std::ofstream fout(filePath);
    fout << out.c_str();  // Write the YAML emitter output to file
    fout.close();
}

MenuCategory* findCategoryByName(std::vector<MenuCategory>& menuData, const std::string& name) {
    auto it = std::find_if(menuData.begin(), menuData.end(), [&name](const MenuCategory& category) {
        return category.name == name;
    });
    if (it != menuData.end()) {
        return &(*it);  // Return a pointer to the existing category
    }
    return nullptr;  // Return nullptr if the category doesn't exist
}



void ConfigMenu::parseLESMenuConfig(const std::filesystem::path& filePath) {

    std::vector<MenuCategory> menuData;
    QString qFilePath = QString::fromStdString(filePath.string());

    QFile file(qFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // can't open file
        return;
    }

    QTextStream in(&file);
    MenuCategory* currentCategory = nullptr;
    std::vector<MenuCategory*> categoryStack;  // Stack to keep track of nested submenus

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith(";")) {
            // Skip comments
            continue;
        } else if (line.startsWith("--")) {
            // Handle separators by adding them as an item
            if (currentCategory != nullptr) {
                MenuItem separator;
                separator.label = "--";
                separator.action = "separator";
                currentCategory->items.push_back(separator);  // Add separator to main category
            }
        } else if (line.startsWith("..")) {
            // Go up one level in the category stack
            if (categoryStack.size() > 1) {
                categoryStack.pop_back();
                currentCategory = categoryStack.back();
            } else {
                currentCategory = nullptr;  // Go back to top level if stack is empty
            }
        } else if (line.startsWith("/")) {
            // Determine submenu depth by counting '/'
            std::string lineStr = line.toStdString();
            std::regex topLevelCategoryRegex("^/(?!nocategory)[^/](.*)");
            std::smatch matches;

            if (std::regex_search(lineStr, matches, topLevelCategoryRegex)) {
                if (matches.size() > 0) {
                    LogHandler::getInstance().info("\n\nmatch");
                    QString categoryName = QString::fromStdString(matches[0].str());
                    // Find or create the top-level category
                    currentCategory = findCategoryByName(menuData, categoryName.toStdString());
                    if (!currentCategory) {
                        // Create a new category if it doesn't exist
                        MenuCategory newCategory;
                        newCategory.name = categoryName.toStdString();
                        menuData.push_back(newCategory);
                        currentCategory = &menuData.back();
                    }
                    categoryStack.clear();
                    categoryStack.push_back(currentCategory);
                }

            } else if (line.startsWith("/nocategory")) {
                // Reset to the top-level category
                currentCategory = nullptr;
                categoryStack.clear();

            } else if (line.startsWith("//")) {
                // Handle subcategory (two slashes for subcategory depth)
                LogHandler::getInstance().info("\n\nstarts with //");
                QString subCategoryName = line.section('/', 2, 2);  // Extract the subcategory name

                // Create a subcategory directly inside the current category's items
                MenuItem subCategoryItem;
                subCategoryItem.label = subCategoryName.toStdString();
                subCategoryItem.subCategory = std::make_shared<MenuCategory>();  // Create subcategory within item
                subCategoryItem.subCategory->name = subCategoryName.toStdString();

                LogHandler::getInstance().info("Parsed subcategory: " + subCategoryName.toStdString());

                // Add subcategory item to the parent category
                currentCategory->items.push_back(subCategoryItem);

                // Set the current subcategory for adding its items
                currentCategory = subCategoryItem.subCategory.get();
                categoryStack.push_back(currentCategory);
            }
        } else if (!line.isEmpty() && currentCategory != nullptr) {
            // Read label and action for the current category
            MenuItem item;
            item.label = line.toStdString();
            item.action = in.readLine().trimmed().toStdString();
            currentCategory->items.push_back(item);  // Add item to main or subcategory
        }
    }

    file.close();

    std::filesystem::path configMenuFilePath =
        std::filesystem::path(std::string(getenv("HOME")))
        / "Documents" / "Ableton" / "User Library"
        / "Remote Scripts" / "LiveImproved" / "config-menu.txt"
    ;
    saveToYAML(menuData, configMenuFilePath);
}

void ConfigMenu::loadConfig() {
    try {

    } catch (const std::exception &e) {
        LogHandler::getInstance().info("Error loading config: " + std::string(e.what()));
    }
}

