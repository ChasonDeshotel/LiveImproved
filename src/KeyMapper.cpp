#include "KeyMapper.h"
#include "LogHandler.h"

KeyMapper::KeyMapper()
    : log_(&LogHandler::getInstance())
    , valid(false) {}

EKeyPress KeyMapper::processKeyPress(const std::string& keypress) {
    log_->info(keypress);
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
        if (temp == "cmd")      kp.cmd   = true;
        if (temp == "shift")    kp.shift = true;
        if (temp == "ctrl")     kp.ctrl  = true;
        if (temp == "alt")      kp.alt   = true;
        if (temp.length() == 1) kp.key   = temp[0];
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

std::string KeyMapper::EKeyPressToString(const EKeyPress& keyPress) {
    std::string result;

    if (keyPress.cmd)   result += "cmd+"   ;
    if (keyPress.ctrl)  result += "ctrl+"  ;
    if (keyPress.alt)   result += "alt+"   ;
    if (keyPress.shift) result += "shift+" ;

    result += keyPress.key;

    return result;
}
