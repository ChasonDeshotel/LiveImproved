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
struct EKeyPressHash {
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

struct EKeyMacro {
    std::vector<EKeyPress> keypresses;
};

#endif
