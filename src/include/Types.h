#ifndef TYPES_H
#define TYPES_H

#include <functional>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <iostream>

#include "KeySender.h"

// Actions
struct NamedActions {
    static const std::unordered_set<std::string>& get() {
        static const std::unordered_set<std::string> namedActions = {
            "load_item"
            , "plugin"
            , "searchbox"
        };
        return namedActions;
    }
};

struct Action {
    std::string actionName;
    std::optional<std::string> arguments;

    Action(const std::string& actionName, const std::optional<std::string>& args = std::nullopt)
        : actionName(actionName), arguments(args) {}

    bool operator==(const Action& other) const {
    return actionName == other.actionName &&
           arguments == other.arguments;
    };
    // TODO Placeholder validation function (optional)
//    void validate() const {
//        // Add validation logic here if needed later
//        if (actionName == "delay") {
//            try {
//                double delayTime = std::stod(arguments);  // Ensure valid number
//            } catch (std::invalid_argument&) {
//                throw std::runtime_error("Invalid argument for delay: expected a number.");
//            }
//        }
//    }
    friend std::ostream& operator<<(std::ostream& os, const Action& action) {
        os << "Action(namedAction=" << action.actionName
           << ", arguments=" << action.arguments.value_or("") << ")";
        return os;
    }
};

// Keyboard
struct NamedKeys {
    static const std::unordered_set<std::string>& get() {
        static const std::unordered_set<std::string> namedKeys = {
              "delete"
            , "enter"
            , "escape"
            , "space"
            , "tab"
            , "backspace"
        };
        return namedKeys;
    }
};

enum Modifier {
    // macOS values
    None    = 0
    , Shift   = 0x00020000
    , Ctrl    = 0x00040000
    , Alt     = 0x00080000
    , Cmd     = 0x00100000
};

// comparisons on modifiers were causing malloc
// thus the attribute packed voodoo
// edit: this might have actually been caused
// by running two instances
struct __attribute__((packed)) EKeyPress {
    bool shift = false;
    bool ctrl  = false;
    bool cmd   = false;
    bool alt   = false;
    std::string key;

    // determine if two EKeyPress objects are identical
    // (all fields must match)
    bool operator==(const EKeyPress& other) const {
    return (ctrl  == other.ctrl  &&
            alt   == other.alt   &&
            shift == other.shift &&
            cmd   == other.cmd   &&
            key   == other.key   );
    }

    friend std::ostream& operator<<(std::ostream& os, const EKeyPress& kp) {
        os << "EKeyPress(ctrl=" << kp.ctrl
           << ", alt=" << kp.alt
           << ", shift=" << kp.shift
           << ", cmd=" << kp.cmd
           << ", key=" << kp.key << ")";
        return os;
    }
};

struct EMacroHash {
    // boost hashing voodoo
    std::size_t operator()(const EKeyPress& k) const {
        std::size_t res = 17;
        res = res * 31 + std::hash<bool>()(k.ctrl);
        res = res * 31 + std::hash<bool>()(k.alt);
        res = res * 31 + std::hash<bool>()(k.shift);
        res = res * 31 + std::hash<bool>()(k.cmd);
        res = res * 31 + std::hash<std::string>()(k.key);
        return res;
    }
};

struct EMacro {
    std::vector<std::variant<EKeyPress, Action>> steps;

    void addKeyPress(const EKeyPress& keyPress) {
        steps.emplace_back(keyPress);
    }

    void addAction(const Action& action) {
        steps.emplace_back(action);
    }

    bool operator==(const EMacro& other) const {
        return steps == other.steps;
    }

    friend std::ostream& operator<<(std::ostream& os, const EMacro& macro) {
        os << "EMacro(steps=[";
        for (const auto& step : macro.steps) {
            std::visit([&os](auto&& arg) { os << arg << ", "; }, step);
        }
        os << "])";
        return os;
    }
};

// GUI
class IWindow {
public:
    virtual ~IWindow() = default;
    virtual void open() = 0;
    virtual void close() = 0;
    virtual void* getWindowHandle() const = 0;
};

enum class Window {
    ContextMenu
};

struct WindowData {
    std::shared_ptr<IWindow> window;
    std::function<void()> callback;
};

struct MenuItem {
    std::string label;
    std::string action;
    std::vector<MenuItem> children;
};

// New Folder (3)
struct Plugin {
    int number;
    std::string name;
    std::string type;
    std::string uri;
};

#endif
