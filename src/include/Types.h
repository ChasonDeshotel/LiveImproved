#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <regex>
#include <memory>
#include <functional>
#include <unordered_map>

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

struct Plugin {
    int number;
    std::string name;
    std::string type;
    std::string uri;
};

enum Modifier {
    // macOS values
    None    = 0
    , Shift   = 0x00020000
    , Ctrl    = 0x00040000
    , Alt     = 0x00080000
    , Cmd     = 0x00100000
};

struct EKeyPress {
    bool shift = false;
    bool ctrl  = false;
    bool cmd   = false;
    bool alt   = false;
    std::vector<Modifier> modifiers;
    std::string key;

    bool operator==(const EKeyPress& other) const {
    return (ctrl  == other.ctrl  &&
            alt   == other.alt   &&
            shift == other.shift &&
            cmd   == other.cmd   &&
            key   == other.key   );
    }
    // Helper function to display the keypress (for debugging)
//    void display() const {
//        for (const auto& mod : modifiers) {
//            std::cout << modifierToString(mod) << "+";
//        }
//        std::cout << key << std::endl;
//    }
//
//    // Convert Modifier enum to string (lowercase)
//    std::string modifierToString(Modifier mod) const {
//        switch (mod) {
//            case Modifier::Cmd: return "cmd";
//            case Modifier::Shift: return "shift";
//            case Modifier::Ctrl: return "ctrl";
//            case Modifier::Alt: return "alt";
//            default: return "";
//        }
//    }
};

struct EKeyPressHash {
    std::size_t operator()(const EKeyPress& k) const {
        // Combine all fields into a single hash
        return ((std::hash<bool>()(k.ctrl) ^ (std::hash<bool>()(k.alt) << 1)) >> 1) ^
               (std::hash<bool>()(k.shift) << 1) ^ (std::hash<bool>()(k.cmd) << 1) ^
               std::hash<std::string>()(k.key);
    }
};

struct EKeyMacro {
    std::vector<EKeyPress> keypresses;
};

#endif
