#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <ranges>

#include "ResponseParser.h"
#include "Types.h"
#include "Utils.h"

ResponseParser::ResponseParser() = default;

ResponseParser::~ResponseParser() = default;

auto ResponseParser::parsePlugins(const std::string& input) -> std::vector<Plugin> {
    std::vector<Plugin> plugins;
    std::vector<std::string> entries = Utils::split(input, '|');

    for (const std::string& entry : entries) {
        std::vector<std::string> fields = Utils::split(entry, ',');
        if (fields.size() == 3) {
            Plugin plugin;
            plugin.number = std::stoi(fields[0]);
            plugin.name = fields[1];
            size_t typePos = fields[2].find('#');
            if (typePos != std::string::npos) {
                plugin.type = fields[2].substr(typePos + 1, 5); // NOLINT Extracts type (e.g., "#AUv2")
                plugin.uri = fields[2].substr(typePos + 6);     // NOLINT Extracts the URI part
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
