#include "KeyMapper.h"
#include "LogHandler.h"
#include "Types.h"

KeyMapper::KeyMapper()
    : log_(&LogHandler::getInstance())
    , valid(false) {}

EKeyPress KeyMapper::processKeyPress(const std::string& keypress) {
    log_->debug("process key: " + keypress);
    if (validateHotkey(keypress)) {
        this->keypress = parseKeyPress(keypress);
        this->valid = true;
        return this->keypress;
    } else {
        log_->warn("not a valid keypress format: " + keypress);
        this->valid = false;
        throw std::runtime_error("not a valid keypress format: " + keypress);
    }
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
    log_->debug("parsing: " + keypress);
    const auto& namedKeys = NamedKeys::get();

    EKeyPress kp;

    if (keypress.length() == 1) {
        kp.key = keypress;
        log_->debug("Key set to: " + kp.key);
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
    log_->debug("key: "   + kp.key);
    log_->debug("shift: " + std::to_string(kp.shift));
    log_->debug("ctrl: "  + std::to_string(kp.ctrl));
    log_->debug("cmd: "   + std::to_string(kp.cmd));
    log_->debug("alt: "   + std::to_string(kp.alt));

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

    // TODO build named key part from NamedKeys
//    std::string pattern = "^((" + modifierPart + "\\+)*[\\p{L}\\d\\-\\.])$"; // p{L} matches a single Unicode letter
   // std::string pattern = "^((" + modifierPart + "\\+)*[a-zA-Z0-9])$"; // p{L} matches a single Unicode letter

    std::string pattern = "^(?:(?:" + modifierPart + ")\\+)*(?:[a-zA-Z0-9]|F[1-9]|F1[0-2]|delete|enter|escape|space|tab|backspace)$";
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
