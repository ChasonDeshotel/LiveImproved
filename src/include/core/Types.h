#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "LogGlobal.h"

// pid_t
#ifdef _WIN32
#include <windows.h>
typedef DWORD pid_t;
#else
#include <sys/types.h>
#include <unistd.h>
#endif

struct ERect {
    int x;
    int y;
    int width;
    int height;
};

// Actions
struct NamedActions {
    static auto get() -> const std::unordered_set<std::string>& {
        static const std::unordered_set<std::string> namedActions = {
            "load_item"
            , "plugin"
            , "searchbox"
            , "closeFocusedPlugin"
            , "closeAllPlugins"
            , "openAllPlugins"
            , "tilePluginWindows"
        };
        return namedActions;
    }
};

struct Action {
    std::string actionName;
    std::optional<std::string> arguments;

    Action(std::string actionName, const std::optional<std::string>& args = std::nullopt)
        : actionName(std::move(actionName)), arguments(args) {}

    auto operator==(const Action& other) const -> bool {
        return actionName == other.actionName &&
               arguments == other.arguments;
    }
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
    friend auto operator<<(std::ostream& os, const Action& action) -> std::ostream& {
        os << "Action(namedAction=" << action.actionName
           << ", arguments=" << action.arguments.value_or("") << ")";
        return os;
    }
};

// Keyboard
struct NamedKeys {
    static auto get() -> const std::unordered_set<std::string>& {
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

enum class KeyState {
    Up
    , Down
    , Unknown
};

// comparisons on modifiers were causing malloc
// thus the attribute packed voodoo
// edit: this might have actually been caused
// by running two instances
struct EKeyPress {
    KeyState state = KeyState::Unknown;
    bool shift = false;
    bool ctrl  = false;
    bool cmd   = false;
    bool alt   = false;
    std::string key;

    // determine if two EKeyPress objects are identical
    // (all fields must match)
    auto operator==(const EKeyPress& other) const -> bool {
        return (ctrl  == other.ctrl  &&
                alt   == other.alt   &&
                shift == other.shift &&
                cmd   == other.cmd   &&
                key   == other.key   );
    }

    friend auto operator<<(std::ostream& os, const EKeyPress& kp) -> std::ostream& {
        os << "EKeyPress(ctrl=" << kp.ctrl
           << ", alt=" << kp.alt
           << ", shift=" << kp.shift
           << ", cmd=" << kp.cmd
           << ", key=" << kp.key << ")";
        return os;
    }

    [[nodiscard]] auto isModifierPressed() const -> bool {
        return shift || ctrl || cmd || alt;
    }

    void print() const {
        logger->info("state\t" + std::to_string(static_cast<int>(state)));
        logger->info("shift\t" + std::string(shift ? "true" : "false"));
        logger->info("ctrl\t" + std::string(ctrl ? "true" : "false"));
        logger->info("cmd\t" + std::string(cmd ? "true" : "false"));
        logger->info("alt\t" + std::string(alt ? "true" : "false"));
        logger->info("key\t" + key);
    }
};

struct EMacroHash {
    // boost hashing voodoo
    // NOLINTBEGIN
    auto operator()(const EKeyPress& k) const -> std::size_t {
        std::size_t res = 17;
        res = res * 31 + std::hash<bool>()(k.ctrl);
        res = res * 31 + std::hash<bool>()(k.alt);
        res = res * 31 + std::hash<bool>()(k.shift);
        res = res * 31 + std::hash<bool>()(k.cmd);
        res = res * 31 + std::hash<std::string>()(k.key);
        return res;
    }
    // NOLINTEND
};

struct EMacro {
    std::vector<std::variant<EKeyPress, Action>> steps;

    void addKeyPress(const EKeyPress& keyPress) {
        steps.emplace_back(keyPress);
    }

    void addAction(const Action& action) {
        steps.emplace_back(action);
    }

    auto operator==(const EMacro& other) const -> bool {
        return steps == other.steps;
    }

    friend auto operator<<(std::ostream& os, const EMacro& macro) -> std::ostream& {
        os << "EMacro(steps=[";
        for (const auto& step : macro.steps) {
            std::visit([&os](auto&& arg) { os << arg << ", "; }, step);
        }
        os << "])";
        return os;
    }

    void print() const {
        for (size_t i = 0; i < steps.size(); ++i) {
            logger->info("Step\t" + std::to_string(i + 1));
            std::visit([this](const auto& step) {
                if constexpr (std::is_same_v<std::decay_t<decltype(step)>, EKeyPress>) {
                    step.print();
                } else if constexpr (std::is_same_v<std::decay_t<decltype(step)>, Action>) {
                    logger->info("Action\t" + step.actionName);
                    if (step.arguments) {
                        logger->info("Arguments\t" + *step.arguments);
                    }
                }
            }, steps[i]);
            logger->info("");
        }
    }
};

// New Folder (3)
struct Plugin {
    int number;
    std::string name;
    std::string type;
    std::string uri;
};
