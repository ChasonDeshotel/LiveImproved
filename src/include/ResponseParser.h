#ifndef RESPONSE_PARSER_H
#define RESPONSE_PARSER_H

#include <string>
#include <vector>

#include "Types.h"

class ApplicationManager;

class ResponseParser {
public:
    ResponseParser(ApplicationManager& appManager);
    ~ResponseParser();

    std::vector<std::string> split(const std::string& str, char delimiter);
    std::vector<std::string> splitStringInPlace(std::string& str, char delimiter);
    std::vector<Plugin> parsePlugins(const std::string& input);
    void sortByName(std::vector<Plugin>& plugins);
    std::vector<Plugin> getUniquePlugins(const std::vector<Plugin>& plugins);

 private:
    ApplicationManager& app_;

};

#endif
