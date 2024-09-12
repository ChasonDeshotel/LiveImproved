#include "KeyMapper.h"
#include "ApplicationManager.h"

KeyMapper::KeyMapper(ApplicationManager& appManager) 
    : app_(appManager), valid(false) {}

EKeyPress KeyMapper::processKeyPress(const std::string& keypress) {
    app_.getLogHandler()->info(keypress);
    if (validateHotkey(keypress)) {
        this->keypress = parseKeyPress(keypress);
        this->valid = true;
    } else {
        this->valid = false;
    }
    return this->keypress;
}

bool KeyMapper::isValid() const {
    return valid;
}

const EKeyPress& KeyMapper::getKeyPress() const {
    return keypress;
}

bool KeyMapper::validateHotkey(const std::string& keypress) const {
    std::string regexPattern = buildRegexPattern();
    std::regex regex(regexPattern);
    return std::regex_match(keypress, regex);
}

EKeyPress KeyMapper::parseKeyPress(const std::string& keypress) const {
    EKeyPress kp;
    std::string temp;
    std::stringstream ss(keypress);

    while (std::getline(ss, temp, '+')) {
        if (temp == "cmd") kp.modifiers.push_back(Modifier::Cmd);
        else if (temp == "shift") kp.modifiers.push_back(Modifier::Shift);
        else if (temp == "ctrl") kp.modifiers.push_back(Modifier::Ctrl);
        else if (temp == "alt") kp.modifiers.push_back(Modifier::Alt);
        else if (temp.length() == 1) kp.key = temp[0];
    }

    return kp;
}

std::string KeyMapper::buildRegexPattern() const {
    std::vector<std::string> modifiers = {
        toLowerCase("Cmd"),
        toLowerCase("Shift"),
        toLowerCase("Ctrl"),
        toLowerCase("Alt")
    };

    std::string modifierPart;
    for (const auto& mod : modifiers) {
        if (modifierPart.size() > 1) {
            modifierPart += "|";
        }
        modifierPart += mod;
    }

    std::string pattern = "^((" + modifierPart + "\\+)*\\p{L})$"; // p{L} matches a single Unicode letter
    return pattern;
}

std::string KeyMapper::toLowerCase(const std::string& input) const {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(), ::tolower);
    return output;
}
