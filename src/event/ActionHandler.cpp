// TODO cross-platform
#ifndef _WIN32
#import <ApplicationServices/ApplicationServices.h>
#endif
#include <unordered_map>
#include <string>

#include "LogGlobal.h"
#include "Types.h"
#include "Utils.h"

#include "IEventHandler.h"
#include "IIPCQueue.h"
#include "ILiveInterface.h"

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "ContextMenu.h"
#include "KeySender.h"
#include "PluginManager.h"
#include "WindowManager.h"

ActionHandler::ActionHandler(
              std::function<std::shared_ptr<IPluginManager>()> pluginManager
              , std::function<std::shared_ptr<WindowManager>()> windowManager
              , std::function<std::shared_ptr<ConfigManager>()> configManager
              , std::function<std::shared_ptr<IIPCQueue>()> ipc
              , std::function<std::shared_ptr<IEventHandler>()> eventHandler
              , std::function<std::shared_ptr<ILiveInterface>()> liveInterface
              )
    : ipc_(std::move(ipc))
    , windowManager_(std::move(windowManager))
    , pluginManager_(std::move(pluginManager))
    , configManager_(std::move(configManager))
    , eventHandler_(std::move(eventHandler))
    , liveInterface_(std::move(liveInterface))
{
    initializeActionMap();
}

ActionHandler::~ActionHandler() = default;

// Initialize the action map
void ActionHandler::initializeActionMap() {
    auto wm = windowManager_();
    // TODO: case insensitive
    // TODO: multiple args (plugin,Sylenth,Serum)

    // NOTE actions must be added here and in Types.h
    actionMap["closeFocusedPlugin"] = [this](const std::optional<std::string>& args) {
        auto liveInterface = liveInterface_();
        liveInterface->closeFocusedPlugin();
    };

    actionMap["closeAllPlugins"] = [this](const std::optional<std::string>& args) {
        auto liveInterface = liveInterface_();
        liveInterface->closeAllPlugins();
    };

    actionMap["openAllPlugins"] = [this](const std::optional<std::string>& args) {
        auto liveInterface = liveInterface_();
        liveInterface->openAllPlugins();
    };

    actionMap["tilePluginWindows"] = [this](const std::optional<std::string>& args) {
        auto liveInterface = liveInterface_();
        liveInterface->tilePluginWindows();
    };

    actionMap["searchbox"] = [this](const std::optional<std::string>& args) {
        auto wm = windowManager_();
        wm->openWindow("SearchBox");
    };

    actionMap["write-request"] = [this](const std::optional<std::string>& args) {
        if (args) {
            auto ipc = ipc_();
            ipc->createRequest(*args);
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
                logger->debug("macro sending keypress");
                KeySender::getInstance().sendKeyPress(item);
            } else if constexpr (std::is_same_v<T, Action>) {
                logger->debug("macro sending action");
                // If it's an Action, execute the action via the action map
                auto it = actionMap.find(item.actionName);
                if (it != actionMap.end()) {
                    std::optional<std::string> optionalArgument = item.arguments;
                    it->second(optionalArgument);
                } else {
                    logger->warn("Unknown action: " + item.actionName);
                }
            }
        }, step);
    }
}

auto ActionHandler::closeWindows() -> bool {
    auto wm = windowManager_();
    wm->closeWindow("ContextMenu");
    wm->closeWindow("SearchBox");

    return false;
}

auto ActionHandler::loadItem(int itemIndex) -> bool {
    auto ipc = ipc_();
    ipc->createRequest("load_item," + std::to_string(itemIndex));

    return false;
}

auto ActionHandler::loadItemByName(const std::string& itemName) -> bool {
    auto ipc = ipc_();
    auto pluginManager = pluginManager_();
    for (const auto& plugin : pluginManager->getPlugins()) {
        if (itemName == plugin.name) {
          ipc->createRequest("load_item," + std::to_string(plugin.number));
          return true;
        }
    }
    return false;
}

auto ActionHandler::handleAction(std::string action) -> void {
    logger->debug("handleAction called");

    // Split by ',' to get individual actions
    std::vector<std::string> actions = Utils::splitString(action, ",");

    if (actions.empty()) {
        logger->warn("No actions provided");
        return;
    }

    // Loop through each action
    for (auto& act : actions) {
        Utils::trim(act);

        // Split by '.' to get the action type and optional argument
        std::vector<std::string> actionParts = Utils::splitString(act, ".", 1);

        if (actionParts.empty()) {
            logger->warn("Malformed action: " + act);
            continue;
        }

        // Get the action type (first part before '.')
        std::string actionType = Utils::trim(actionParts[0]);
        logger->debug("Processing actionType: " + actionType);

        // Try to find the action handler in the map
        auto actionHandler = actionMap.find(actionType);
        if (actionHandler != actionMap.end()) {
            // Check if there is an argument (second part after '.')
            if (actionParts.size() > 1) {
                std::string args = Utils::trim(actionParts[1]);
                logger->debug("Action has argument: " + args);

                // Call the handler with the argument
                actionHandler->second(args);
            } else {
                // No argument, pass std::nullopt
                logger->debug("No argument provided for action: " + actionType);
                actionHandler->second(std::nullopt);
            }
        } else {
            // No handler found for this action type
            logger->warn("No handler found for action type: " + actionType);
        }
    }
}

// returns: bool: shouldPassEvent -- should the original event be passed
// through to the calling function or should the original input be blocked
// TODO: return shouldBlock ASAP and do the needful from dispatch_async

// TODO cross-platform - send flags as another KeyPress object since CGEventFlags
// doesn't exist on Windows
auto ActionHandler::handleKeyEvent(EKeyPress pressedKey) -> bool {
    auto config = configManager_();
    auto wm = windowManager_();
//    logger->info("action handler: Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

    // static cast probably not necessary

    std::unordered_map<EKeyPress, EMacro, EMacroHash> remap = config->getRemap();

    //logger->debug("searching map for pressed cmd: "   + std::to_string(pressedKey.cmd));
    //logger->debug("searching map for pressed ctrl: "  + std::to_string(pressedKey.ctrl));
    //logger->debug("searching map for pressed alt: "   + std::to_string(pressedKey.alt));
    //logger->debug("searching map for pressed shift: " + std::to_string(pressedKey.shift));
    //logger->debug("searching map for pressed sent: "  + pressedKey.key);

    // key remaps
    auto it = remap.find(pressedKey);
    if (it != remap.end()) {
        executeMacro(it->second);
        return false;
    } else {
        logger->warn("Key not found in remap: " + pressedKey.key);
    }

    // when the menu is open, do not send keypresses to Live
    // or it activates your hotkeys
    if (wm->isWindowOpen("SearchBox")) {
        logger->debug("is open, do not pass keys to Live");
        return false;
    }

    // if we meet no criteria,
    // the original event should
    // be passed to the original caller
    return true;
}

auto ActionHandler::handleDoubleRightClick() -> void {
    auto wm = windowManager_();
    // TODO cross-platform
    #ifndef _WIN32
    dispatch_async(dispatch_get_main_queue(), ^{
        wm->openWindow("ContextMenu");
    });
	#endif
}
