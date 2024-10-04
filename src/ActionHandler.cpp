// TODO cross-platform
#ifndef _WIN32
	#include <ApplicationServices/ApplicationServices.h>
#endif
#include <unordered_map>
#include <iostream>
#include <string>

#include "LogHandler.h"
#include "Types.h"
#include "Utils.h"

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "ContextMenu.h"
#include "IPC.h"
#include "KeySender.h"
#include "PluginManager.h"
#include "WindowManager.h"

ActionHandler::ActionHandler(
              std::function<std::shared_ptr<ILogHandler>()> logHandler
              , std::function<std::shared_ptr<IPluginManager>()> pluginManager
              , std::function<std::shared_ptr<WindowManager>()> windowManager
              , std::function<std::shared_ptr<ConfigManager>()> configManager
              , std::function<std::shared_ptr<IIPC>()> ipc
              )
    : logHandler_(std::move(logHandler))
    , ipc_(std::move(ipc))
    , windowManager_(std::move(windowManager))
    , pluginManager_(std::move(pluginManager))
    , configManager_(std::move(configManager))
{
    initializeActionMap();
}

ActionHandler::~ActionHandler() {}

// Define a function type for action handlers
using ActionHandlerFunction = std::function<void(const std::optional<std::string>& args)>;

// Define a map to link action strings to methods
std::unordered_map<std::string, ActionHandlerFunction> actionMap;

// Initialize the action map
void ActionHandler::initializeActionMap() {
    // TODO: case insensitive
    // TODO: multiple args (plugin,Sylenth,Serum)

    // NOTE actions must be added here and in Types.h
    actionMap["searchbox"] = [this](const std::optional<std::string>& args) {
        windowManager()->openWindow("SearchBox");
    };
    actionMap["write-request"] = [this](const std::optional<std::string>& args) {
        if (args) {
            ipc()->writeRequest(*args);
        } else {
            throw std::runtime_error("write-request action requires an argument");
        }
    };

    actionMap["plugin"] = [this](const std::optional<std::string>& args) {
        if (args) {
            this->loadItemByName(*args);
        } else {
            throw std::runtime_error("plugin action requires an argument");
        }
    };
}

void ActionHandler::executeMacro(const EMacro& macro) {
//    for (const auto& key : macro.keypresses) {
//        log_.info("ActionHandler:: execMacro cmd: "   + std::to_string(key.cmd));
//        log_.info("ActionHandler:: execMacro ctrl: "  + std::to_string(key.ctrl));
//        log_.info("ActionHandler:: execMacro alt: "   + std::to_string(key.alt));
//        log_.info("ActionHandler:: execMacro shift: " + std::to_string(key.shift));
//        log_.info("ActionHandler:: execMacro sent: "  + key.key);
//    }

    for (const auto& step : macro.steps) {
        // Use std::visit to handle the variant type (EKeyPress or Action)
        std::visit([&](auto&& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, EKeyPress>) {
                log()->debug("macro sending keypress");
                KeySender::getInstance().sendKeyPress(item);
            } else if constexpr (std::is_same_v<T, Action>) {
                log()->debug("macro sending action");
                // If it's an Action, execute the action via the action map
                auto it = actionMap.find(item.actionName);
                if (it != actionMap.end()) {
                    std::optional<std::string> optionalArgument = item.arguments;
                    it->second(optionalArgument);
                } else {
                    log()->warn("Unknown action: " + item.actionName);
                }
            }
        }, step);
    }
}

bool ActionHandler::closeWindows() {
    windowManager()->closeWindow("ContextMenu");
    windowManager()->closeWindow("SearchBox");

    return false;
}

bool ActionHandler::loadItem(int itemIndex) {
    ipc()->writeRequest("load_item," + std::to_string(itemIndex));
    return false;
}

bool ActionHandler::loadItemByName(const std::string& itemName) {
    for (const auto& plugin : pluginManager()->getPlugins()) {
        if (itemName == plugin.name) {
          ipc()->writeRequest("load_item," + std::to_string(plugin.number));
          return true;
        }
    }
    return false;
}

void ActionHandler::handleAction(const std::string action) {
    log()->debug("handleAction called");
    std::vector<std::string> actionParts = Utils::splitString(action, ",", 1);

      if (actionParts.empty()) {
        log()->warn("No action provided");
        return;
    }

    std::string actionType = actionParts[0];
    std::string args = actionParts[1];

    auto actionHandler = actionMap.find(actionType);
    if (actionHandler != actionMap.end()) {
        // Call the corresponding method with arguments
        actionHandler->second(args);
    } else {
        log()->warn("No handler found for action type: " + actionType);
    }
}

// returns: bool: shouldPassEvent -- should the original event be passed
// through to the calling function or should the original input be blocked
// TODO: return shouldBlock ASAP and do the needful from dispatch_async

// TODO cross-platform - send flags as another KeyPress object since CGEventFlags
// doesn't exist on Windows
bool ActionHandler::handleKeyEvent(EKeyPress pressedKey) {
//    app_.getLogHandler()->info("action handler: Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

    // static cast probably not necessary

    std::unordered_map<EKeyPress, EMacro, EMacroHash> remap = configManager()->getRemap();

    log()->debug("searching map for pressed cmd: "   + std::to_string(pressedKey.cmd));
    log()->debug("searching map for pressed ctrl: "  + std::to_string(pressedKey.ctrl));
    log()->debug("searching map for pressed alt: "   + std::to_string(pressedKey.alt));
    log()->debug("searching map for pressed shift: " + std::to_string(pressedKey.shift));
    log()->debug("searching map for pressed sent: "  + pressedKey.key);

    // key remaps
    auto it = remap.find(pressedKey);
    if (it != remap.end()) {
        executeMacro(it->second);
        return false;
    } else {
        log()->warn("Key not found in remap: " + pressedKey.key);
    }

    // when the menu is open, do not send keypresses to Live
    // or it activates your hotkeys
    if (windowManager()->isWindowOpen("SearchBox")) {
        log()->debug("is open, do not pass keys to Live");
        return false;
    }

    // if we meet no criteria,
    // the original event should
    // be passed to the original caller
    return true;
}

void ActionHandler::handleDoubleRightClick() {
    // TODO cross-platform
    #ifndef _WIN32
		dispatch_async(dispatch_get_main_queue(), ^{
			windowManager()->openWindow("ContextMenu");
		});
	#endif
}
