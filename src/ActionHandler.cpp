// TODO cross-platform
#ifndef _WIN32
	#include <ApplicationServices/ApplicationServices.h>
#endif
#include <unordered_map>
#include <iostream>
#include <string>

#include "LogHandler.h"
#include "Types.h"

#include "ActionHandler.h"
#include "ConfigManager.h"
#include "ContextMenu.h"
#include "IPC.h"
#include "KeySender.h"
#include "PluginManager.h"
#include "WindowManager.h"

ActionHandler::ActionHandler(
              std::shared_ptr<ILogHandler> logHandler
              , std::shared_ptr<IPluginManager> pluginManager
              , std::shared_ptr<WindowManager> windowManager
              , std::shared_ptr<ConfigManager> configManager
              , std::shared_ptr<IIPC> ipc
              )
    : log_(std::move(logHandler))
    , ipc_(std::move(ipc))
    , windowManager_(std::move(windowManager))
    , pluginManager_(std::move(pluginManager))
    , configManager_(std::move(configManager))
{
    initializeActionMap();
}

ActionHandler::~ActionHandler() {}

void ActionHandler::init() {
    // should do the mapping / read config or something
}

// TODO DRY violation
std::vector<std::string> splitString(const std::string& str, const std::string& delimiter = ",", size_t maxSplits = std::string::npos) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = 0;
    size_t splits = 0;

    while ((end = str.find(delimiter, start)) != std::string::npos && splits < maxSplits) {
        tokens.push_back(str.substr(start, end - start));  // Add substring before the delimiter
        start = end + delimiter.length();                  // Move past the delimiter
    }

    tokens.push_back(str.substr(start));  // Add the remaining part after the last delimiter
    return tokens;
}

// need to block all events when 
// app_.getGUISearchBox()->isOpen() = true

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
        windowManager_->openWindow("SearchBox");
    };
    actionMap["write-request"] = [this](const std::optional<std::string>& args) {
        if (args) {
            ipc_->writeRequest(*args);
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
   // app_.getIPC()->writeRequest("RELOAD");
   // app_.refreshPluginCache();
   // windowManager_()->openWindow("ContextMenu");
   // windowManager_()->closeWindow("ContextMenu");
   // closeWindows();
}

void ActionHandler::executeMacro(const EMacro& macro) {
//    for (const auto& key : macro.keypresses) {
//        log_.info("ActionHandler:: execMacro cmd: "   + std::to_string(key.cmd));
//        log_.info("ActionHandler:: execMacro ctrl: "  + std::to_string(key.ctrl));
//        log_.info("ActionHandler:: execMacro alt: "   + std::to_string(key.alt));
//        log_.info("ActionHandler:: execMacro shift: " + std::to_string(key.shift));
//        log_.info("ActionHandler:: execMacro sent: "  + key.key);
//    }
//    macro.sendKeys();  // Send each individual key press
    for (const auto& step : macro.steps) {
        // Use std::visit to handle the variant type (EKeyPress or Action)
        std::visit([&](auto&& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, EKeyPress>) {
                log_->debug("macro sending keypress");
                KeySender::getInstance().sendKeyPress(item);
            } else if constexpr (std::is_same_v<T, Action>) {
                log_->debug("macro sending action");
                // If it's an Action, execute the action via the action map
                auto it = actionMap.find(item.actionName);
                if (it != actionMap.end()) {
                    std::optional<std::string> optionalArgument = item.arguments;
                    it->second(optionalArgument);
                } else {
                    log_->warn("Unknown action: " + item.actionName);
                }
            }
        }, step);
    }
}

bool ActionHandler::closeWindows() {
    windowManager_->closeWindow("ContextMenu");
    windowManager_->closeWindow("SearchBox");

// TODO: handle in SearchBox close method
// if close is called while text box is populated, just
// clear textbox; otherwise, close
//    if (app_.getGUISearchBox()->isOpen()) {
//        if (app_.getGUISearchBox()->getSearchTextLength()) {
//            app_.getGUISearchBox()->clearSearchText();
//        } else {
//            app_.getGUISearchBox()->closeSearchBox();
//        }
//    }
    return false;
}

bool ActionHandler::loadItem(int itemIndex) {
    ipc_->writeRequest("load_item," + std::to_string(itemIndex));
    return false;
}

bool ActionHandler::loadItemByName(const std::string& itemName) {
    for (const auto& plugin : pluginManager_->getPlugins()) {
        if (itemName == plugin.name) {
          ipc_->writeRequest("load_item," + std::to_string(plugin.number));
          return true;
        }
    }
    return false;
}

void ActionHandler::handleAction(const std::string action) {
    log_->debug("handleAction called");
    std::vector<std::string> actionParts = splitString(action, ",", 1);

      if (actionParts.empty()) {
        log_->warn("No action provided");
        return;
    }

    std::string actionType = actionParts[0];
    std::string args = actionParts[1];

    auto actionHandler = actionMap.find(actionType);
    if (actionHandler != actionMap.end()) {
        // Call the corresponding method with arguments
        actionHandler->second(args);
    } else {
        log_->warn("No handler found for action type: " + actionType);
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

    std::unordered_map<EKeyPress, EMacro, EMacroHash> remap = configManager_->getRemap();

    log_->debug("searching map for pressed cmd: "   + std::to_string(pressedKey.cmd));
    log_->debug("searching map for pressed ctrl: "  + std::to_string(pressedKey.ctrl));
    log_->debug("searching map for pressed alt: "   + std::to_string(pressedKey.alt));
    log_->debug("searching map for pressed shift: " + std::to_string(pressedKey.shift));
    log_->debug("searching map for pressed sent: "  + pressedKey.key);

    // key remaps
    auto it = remap.find(pressedKey);
    if (it != remap.end()) {
        executeMacro(it->second);
        return false;
    } else {
        log_->warn("Key not found in remap: " + pressedKey.key);
    }

//        } else if (keyString == "Escape") {

            // hjkl navigation
//            case 4:
//              if (app_.getGUISearchBox()->isOpen()) {
//                  return true;
//              } else {
//                  app_.getKeySender()->sendKeyPress(123, 256);
//                  return false;
//              }
//            case 38:
//              if (app_.getGUISearchBox()->isOpen()) {
//                  return true;
//              } else {
//                  app_.getKeySender()->sendKeyPress(125, 256);
//                  return false;
//              }
//            case 40:
//              if (app_.getGUISearchBox()->isOpen()) {
//                  return true;
//              } else {
//                  app_.getKeySender()->sendKeyPress(126, 256);
//                  return false;
//              }
//            case 37:
//              if (app_.getGUISearchBox()->isOpen()) {
//                  return true;
//              } else {
//                  app_.getKeySender()->sendKeyPress(124, 256);
//                  return false;
//              }


    // when the menu is open, do not send keypresses to Live
    // or it activates your hotkeys
    if (windowManager_->isWindowOpen("SearchBox")) {
        log_->debug("is open, do not pass keys to Live");
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
			windowManager_->openWindow("ContextMenu");
		});
	#endif
}

// move to ActionHandler
//				introspect();
//        if (type == kCGEventKeyDown && keyCode == 18 && (CGEventGetFlags(event))) {
//            log_.info("sending new key event");
//
//            CGEventRef newKeyDownEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)46, true); // 37 is the keyCode for 'L'
//						CGEventFlags flags = kCGEventFlagMaskCommand | kCGEventFlagMaskShift;
//            CGEventSetFlags(newKeyDownEvent, flags);
//            CGEventPost(kCGAnnotatedSessionEventTap, newKeyDownEvent);
//
//            CGEventRef newKeyUpEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)46, false); // Key up event
//						CGEventSetFlags(newKeyDownEvent, flags);
//            CGEventPost(kCGAnnotatedSessionEventTap, newKeyUpEvent);
//
//            CFRelease(newKeyDownEvent);
//            CFRelease(newKeyUpEvent);
//
//            return NULL; // block the origianl event
//        }
//		}
//
//		return event;
//}


