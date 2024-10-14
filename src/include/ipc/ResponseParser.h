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
    ResponseParser &operator=(const ResponseParser &) = default;
    ResponseParser &operator=(ResponseParser &&) = delete;

    auto parsePlugins(const std::string& input) -> std::vector<Plugin>;
    auto sortByName(std::vector<Plugin>& plugins) -> void;
    auto getUniquePlugins(const std::vector<Plugin>& plugins) -> std::vector<Plugin>;
};
