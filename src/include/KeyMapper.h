#ifndef KEY_MAPPER_H
#define KEY_MAPPER_H

#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <algorithm>

#include "Types.h"

class ApplicationManager;

class KeyMapper {
public:
    KeyMapper(ApplicationManager& appManager);

    // Method to process a keypress string and return a KeyPress object
    EKeyPress processKeyPress(const std::string& keypress);

    bool isValid() const;
    const EKeyPress& getKeyPress() const;

private:
    ApplicationManager& app_;
    bool valid;
    EKeyPress keypress;

    bool validateHotkey(const std::string& keypress) const;
    EKeyPress parseKeyPress(const std::string& keypress) const;
    std::string buildRegexPattern() const;
    std::string toLowerCase(const std::string& input) const;
};


#endif 
