#pragma once

#include <string>
#include <vector>

class Plugin;

class ResponseParser {
public:
    ResponseParser();
    ~ResponseParser();

    ResponseParser(const ResponseParser &) = default;
    ResponseParser(ResponseParser &&) = delete;
    auto operator=(const ResponseParser &) -> ResponseParser & = default;
    auto operator=(ResponseParser &&) -> ResponseParser & = delete;

    auto parsePlugins(const std::string& input) -> std::vector<Plugin>;
    auto sortByName(std::vector<Plugin>& plugins) -> void;
    auto getUniquePlugins(const std::vector<Plugin>& plugins) -> std::vector<Plugin>;

private:
    std::unordered_map<std::string, int> typePriority = { {"VST3", 1}, {"AUv2", 2}, {"VST2", 3} };
};
