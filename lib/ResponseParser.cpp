#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <iostream>

#include "ResponseParser.h"
#include "ApplicationManager.h"
#include "types/Plugin.h"

ResponseParser::ResponseParser(ApplicationManager& appManager)
    : app_(appManager)
{}

ResponseParser::~ResponseParser() {}

std::vector<std::string> ResponseParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::vector<std::string> ResponseParser::splitStringInPlace(std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    char* start = &str[0];  // Pointer to the beginning of the string
    char* end = start;

    while (*end != '\0') {  // Loop until the end of the string
        if (*end == delimiter) {
            *end = '\0';  // Replace the delimiter with a null terminator
            tokens.push_back(start);  // Store the start pointer as a string in the vector
            start = end + 1;  // Move the start pointer to the next character
        }
        end++;
    }

    tokens.push_back(start);  // Add the last token after the loop ends

    return tokens;
}


std::vector<Plugin> ResponseParser::parsePlugins(const std::string& input) {
    std::vector<Plugin> plugins;
    std::vector<std::string> entries = split(input, '|');

    for (const std::string& entry : entries) {
        std::vector<std::string> fields = split(entry, ',');
        if (fields.size() == 3) {
            Plugin plugin;
            plugin.number = std::stoi(fields[0]);
            plugin.name = fields[1];
            size_t typePos = fields[2].find('#');
            if (typePos != std::string::npos) {
                plugin.type = fields[2].substr(typePos + 1, 5); // Extracts type (e.g., "#AUv2")
                plugin.uri = fields[2].substr(typePos + 6);     // Extracts the URI part
            } else {
                plugin.uri = fields[2];
            }
            plugins.push_back(plugin);
        }
    }

    plugins = getUniquePlugins(plugins);

    std::sort(plugins.begin(), plugins.end(), [](const Plugin& a, const Plugin& b) {
        std::string aLower = a.name;
        std::string bLower = b.name;
        std::transform(aLower.begin(), aLower.end(), aLower.begin(), ::tolower);
        std::transform(bLower.begin(), bLower.end(), bLower.begin(), ::tolower);
        return aLower < bLower;
    });

    return plugins;
}

std::vector<Plugin> ResponseParser::getUniquePlugins(const std::vector<Plugin>& plugins) {
    std::unordered_map<std::string, Plugin> uniquePlugins;
    std::unordered_map<std::string, int> typePriority = { {"VST3", 1}, {"AUv2", 2}, {"VST2", 3} };

    for (const auto& plugin : plugins) {
        if (uniquePlugins.find(plugin.name) == uniquePlugins.end()) {
            uniquePlugins[plugin.name] = plugin;
        } else {
            const auto& existingPlugin = uniquePlugins[plugin.name];
            if (typePriority[plugin.type] < typePriority[existingPlugin.type]) {
                uniquePlugins[plugin.name] = plugin;
            }
        }
    }

    std::vector<Plugin> result;
    for (const auto& pair : uniquePlugins) {
        result.push_back(pair.second);
    }
    
    return result;
}
