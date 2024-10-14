#pragma once

#include <string>
#include <vector>

class Plugin;

class ResponseParser {
public:
    ResponseParser();
    ~ResponseParser();

    auto parsePlugins(const std::string& input) -> std::vector<Plugin>;
    auto sortByName(std::vector<Plugin>& plugins) -> void;
    auto getUniquePlugins(const std::vector<Plugin>& plugins) -> std::vector<Plugin>;
};
