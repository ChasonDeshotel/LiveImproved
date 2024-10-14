#include <regex>
#include <sstream>

#include "LogGlobal.h"
#include "Types.h"

#include "KeyMapper.h"

KeyMapper::KeyMapper()
    : valid(false)
{}

auto KeyMapper::processKeyPress(const std::string& keypress) -> EKeyPress {
    logger->debug("process key: " + keypress);
    if (validateHotkey(keypress)) {
        this->keypress = parseKeyPress(keypress);
        this->valid = true;
        return this->keypress;
    } else {
        logger->warn("not a valid keypress format: " + keypress);
        this->valid = false;
        throw std::runtime_error("not a valid keypress format: " + keypress);
    }
}

auto KeyMapper::isValid() const -> bool {
    return valid;
}

auto KeyMapper::getKeyPress() const -> const EKeyPress& {
    return keypress;
}

auto KeyMapper::validateHotkey(const std::string& keypress) const -> bool {
    std::string regexPattern = buildRegexPattern();
    std::regex regex(regexPattern);
    return std::regex_match(keypress, regex);
}

auto KeyMapper::parseKeyPress(const std::string& keypress) const -> EKeyPress {
    logger->debug("parsing: " + keypress);
    const auto& namedKeys = NamedKeys::get();

    EKeyPress kp;

    if (keypress.length() == 1) {
        kp.key = keypress;
        logger->debug("Key set to: " + kp.key);
        return kp;
    }

    std::string temp;
    std::stringstream ss(keypress);

    while (std::getline(ss, temp, '+')) {
        if (temp == "shift")         kp.shift = true;
        else if (temp == "cmd")      kp.cmd   = true;
        else if (temp == "ctrl")     kp.ctrl  = true;
        else if (temp == "alt")      kp.alt   = true;
        else if (temp.length() == 1) kp.key   = temp[0];
        else if (namedKeys.find(temp) != namedKeys.end()) {
            kp.key = temp;
        }
    }
    logger->debug("key: "   + kp.key);
    logger->debug("shift: " + std::to_string(kp.shift));
    logger->debug("ctrl: "  + std::to_string(kp.ctrl));
    logger->debug("cmd: "   + std::to_string(kp.cmd));
    logger->debug("alt: "   + std::to_string(kp.alt));

    return kp;
}

auto KeyMapper::buildRegexPattern() const -> std::string {
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

    // TODO build named key part from NamedKeys
//    std::string pattern = "^((" + modifierPart + "\\+)*[\\p{L}\\d\\-\\.])$"; // p{L} matches a single Unicode letter
   // std::string pattern = "^((" + modifierPart + "\\+)*[a-zA-Z0-9])$"; // p{L} matches a single Unicode letter

    std::string pattern = "^(?:(?:" + modifierPart + ")\\+)*(?:[a-zA-Z0-9]|F[1-9]|F1[0-2]|delete|enter|escape|space|tab|backspace)$";
    return pattern;
}

auto KeyMapper::toLowerCase(const std::string& input) const -> std::string {
    std::string output = input;
    std::transform(output.begin(), output.end(), output.begin(), ::tolower);
    return output;
}

auto KeyMapper::EKeyPressToString(const EKeyPress& keyPress) -> std::string {
    std::string result;

    if (keyPress.cmd)   result += "cmd+"   ;
    if (keyPress.ctrl)  result += "ctrl+"  ;
    if (keyPress.alt)   result += "alt+"   ;
    if (keyPress.shift) result += "shift+" ;

    result += keyPress.key;

    return result;
}
