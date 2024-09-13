#ifndef TYPES_H
#define TYPES_H

#include <functional>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "KeySender.h"

// Lim
struct NamedActions {
    static const std::unordered_map<std::string, std::string>& get() {
        static const std::unordered_map<std::string, std::string> namedActions = {
            {"load_item", "load_item"},
            {"save", "save"},
            {"delay", "delay"},  // The value "delay" might be used to parse custom delays
            // Add more actions as needed
        };
        return namedActions;
    }
};

enum class ActionType {
    LoadItem
    , Save
    , Delay
};

enum class ArgType {
    None
    , String
    , Double
    , Int
};


struct Action {
    std::string actionName;
    std::optional<std::string> arguments;

    Action(const std::string& actionName, const std::optional<std::string>& args = std::nullopt)
        : actionName(actionName), arguments(args) {}

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
};

// Keyboard
struct NamedKeys {
    static const std::unordered_map<std::string, std::string>& get() {
        static const std::unordered_map<std::string, std::string> namedKeys = {
            {"delete", "delete"},
            {"enter", "enter"},
            {"escape", "escape"},
            {"space", "space"},
            {"tab", "tab"},
            {"backspace", "backspace"},
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
};

// boost hashing voodoo
struct EMacroHash {
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
};

struct Plugin {
    int number;
    std::string name;
    std::string type;
    std::string uri;
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

#endif
