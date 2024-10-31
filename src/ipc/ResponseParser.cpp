#include <algorithm>
#include <cctype>
#include <unordered_map>

#include "ResponseParser.h"
#include "Types.h"
#include "Utils.h"

ResponseParser::ResponseParser() = default;

ResponseParser::~ResponseParser() = default;

auto ResponseParser::parsePlugins(const std::string& input) -> std::vector<Plugin> {
    std::vector<Plugin> plugins;
    std::vector<std::string> entries;

    // Check if the input contains a pipe character
    if (input.find('|') != std::string::npos) {
        entries = Utils::split(input, '|');
    } else {
        // If no pipe, treat the entire input as a single entry
        entries.push_back(input);
    }

    for (const std::string& entry : entries) {
        std::vector<std::string> fields = Utils::split(entry, ',');
        if (fields.size() == 3) {
            Plugin plugin;
            plugin.number = std::stoi(fields[0]) + 1; // Add 1 to make it 1-based
            plugin.name = fields[1];
            size_t typeStartPos = fields[2].find('#');
            if (typeStartPos != std::string::npos) {
                size_t typeEndPos = fields[2].find(':', typeStartPos);
                if (typeEndPos != std::string::npos) {
                    plugin.type = fields[2].substr(typeStartPos + 1, typeEndPos - typeStartPos - 1);
                    plugin.uri = fields[2].substr(typeEndPos + 1);
                }
            } else {
                plugin.uri = fields[2];
            }
            plugins.push_back(plugin);
        }
    }

    plugins = getUniquePlugins(plugins);

    std::ranges::sort(plugins, [](const Plugin& a, const Plugin& b) {
        std::string aLower = a.name;
        std::string bLower = b.name;
        std::ranges::transform(aLower, aLower.begin(), ::tolower);
        std::ranges::transform(bLower, bLower.begin(), ::tolower);
        return aLower < bLower;
    });

    return plugins;
}

auto ResponseParser::getUniquePlugins(const std::vector<Plugin>& plugins) -> std::vector<Plugin> {
    std::unordered_map<std::string, Plugin> uniquePlugins;

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
