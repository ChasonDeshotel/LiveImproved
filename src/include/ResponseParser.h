#pragma once

#include <string>
#include <vector>

class Plugin;

class ResponseParser {
public:
    ResponseParser();
    ~ResponseParser();

    std::vector<std::string> split(const std::string& str, char delimiter);
    std::vector<std::string> splitStringInPlace(std::string& str, char delimiter);
    std::vector<Plugin> parsePlugins(const std::string& input);
    void sortByName(std::vector<Plugin>& plugins);
    std::vector<Plugin> getUniquePlugins(const std::vector<Plugin>& plugins);
};
