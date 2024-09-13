#include <ApplicationServices/ApplicationServices.h>
#include <unordered_map>
#include <iostream>
#include <string>

#include "Types.h"

#include "ActionHandler.h"
#include "ApplicationManager.h"
#include "ContextMenu.h"

#include "KeySender.h"

ActionHandler::ActionHandler(ApplicationManager& appManager)
    : app_(appManager)
    , log_(appManager.getLogHandler())
    , km_(new KeyMapper())
{
    initializeActionMap();
}

ActionHandler::~ActionHandler() {}

void ActionHandler::init() {
    // should do the mapping / read config or something
}

// Helper function to split string by delimiter
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
using ActionHandlerFunction = std::function<void(const std::string&)>;

// Define a map to link action strings to methods
std::unordered_map<std::string, ActionHandlerFunction> actionMap;

// Initialize the action map
void ActionHandler::initializeActionMap() {
    // TODO: case insensitive
    // TODO: multiple args (plugin,Sylenth,Serum)
    actionMap["gui-searchbox"] = [this](const std::string& args) { 
        app_.getWindowManager()->openWindow("SearchBox");
    };
    actionMap["write-request"] = [this](const std::string& args) {
        app_.getIPC()->writeRequest(args);
    };

//    actionMap["keypress"] = [this](const std::string& args) { this->sendKeypress(args); };
    actionMap["plugin"]   = [this](const std::string& args) { this->loadItemByName(args); };
}

void ActionHandler::sendKeypress(EKeyMacro macro) {
    for (const auto& key : macro.keypresses) {
        log_->info("ActionHandler:: sendKeypress cmd: "   + std::to_string(key.cmd));
        log_->info("ActionHandler:: sendKeypress ctrl: "  + std::to_string(key.ctrl));
        log_->info("ActionHandler:: sendKeypress alt: "   + std::to_string(key.alt));
        log_->info("ActionHandler:: sendKeypress shift: " + std::to_string(key.shift));
        log_->info("ActionHandler:: sendKeypress sent: "  + key.key);
    }
    macro.sendKeys();  // Send each individual key press
}

bool ActionHandler::closeWindows() {
    app_.getWindowManager()->closeWindow("ContextMenu");
    app_.getWindowManager()->closeWindow("SearchBox");

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
    app_.getIPC()->writeRequest("load_item," + std::to_string(itemIndex));
    return false;
}

bool ActionHandler::loadItemByName(const std::string& itemName) {
    for (const auto& plugin : app_.getPlugins()) {
        if (itemName == plugin.name) {
          //item->setData(Qt::UserRole, static_cast<int>(plugin.number));
          app_.getIPC()->writeRequest("load_item," + std::to_string(plugin.number));
        }
    }
    return false;
}

// TODO: find a better syntax to send multiple commands
// maybe action.args,action2.args2
void ActionHandler::handleAction(const std::string action) {
    log_->info("handleAction called");
    std::vector<std::string> actionParts = splitString(action, ",", 1);

      if (actionParts.empty()) {
        log_->info("No action provided");
        return;
    }

    std::string actionType = actionParts[0];
    std::string args = actionParts[1];

    auto actionHandler = actionMap.find(actionType);
    if (actionHandler != actionMap.end()) {
        // Call the corresponding method with arguments
        actionHandler->second(args);
    } else {
        log_->info("No handler found for action type: " + actionType);
    }
}

// returns: bool: shouldPassEvent -- should the original event be passed
// through to the calling function or should the original input be blocked
// TODO: return shouldBlock ASAP and do the needful from dispatch_async
bool ActionHandler::handleKeyEvent(std::string keyString, CGEventFlags flags, std::string type) {
//    app_.getLogHandler()->info("action handler: Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

    bool isShiftPressed = static_cast<bool>(flags & Shift) != 0;
    bool isCtrlPressed  = static_cast<bool>(flags & Ctrl ) != 0;
    bool isCmdPressed   = static_cast<bool>(flags & Cmd  ) != 0;
    bool isAltPressed   = static_cast<bool>(flags & Alt  ) != 0;

    //app_.getLogHandler()->info("action handler: Key event: " + type + ", Key string: " + keyString + ", Modifiers: " + std::to_string(flags));
    //app_.getLogHandler()->info("isShiftPressed: " + std::to_string(isShiftPressed));
    //app_.getLogHandler()->info("isCtrlPressed: " + std::to_string(isCtrlPressed));
    //app_.getLogHandler()->info("isCmdPressed: " + std::to_string(isCmdPressed));
    //app_.getLogHandler()->info("isAltPressed: " + std::to_string(isAltPressed));

    std::unordered_map<EKeyPress, EKeyMacro, EKeyPressHash> remap = app_.getConfigManager()->getRemap();

    EKeyPress kp;
    kp.shift = isShiftPressed;
    kp.ctrl  = isCtrlPressed;
    kp.cmd   = isCmdPressed;
    kp.alt   = isAltPressed;
    kp.key   = keyString;

    log_->info("searching map for pressed cmd: "   + std::to_string(kp.cmd));
    log_->info("searching map for pressed ctrl: "  + std::to_string(kp.ctrl));
    log_->info("searching map for pressed alt: "   + std::to_string(kp.alt));
    log_->info("searching map for pressed shift: " + std::to_string(kp.shift));
    log_->info("searching map for pressed sent: "  + kp.key);
    // key remaps
    auto it = remap.find(kp);
    if (it != remap.end()) {
        sendKeypress(it->second);
        return false;
    } else {
        log_->info("Key not found in remap: " + keyString);
    }

    if (type == "keyDown") {
        if (keyString == "1") {
              return false;
        } else if (keyString == "3") {
              app_.getIPC()->writeRequest("RELOAD");
              return false;
        } else if (keyString == "4") {
              app_.refreshPluginCache();
              return false;
        } else if (keyString == "8") {
              app_.getWindowManager()->openWindow("ContextMenu");
              return false;
        } else if (keyString == "7") {
              app_.getWindowManager()->closeWindow("ContextMenu");
              return false;
        } else if (keyString == "Escape") {
              closeWindows();
              return false;
        } else {
//              } else {
//                  return true;
//              }

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

              return true;
        }

        // when the menu is open, do not send keypresses to Live
        // or it activates your hotkeys
        if (app_.getWindowManager()->isWindowOpen("SearchBox")) {
            log_->info("is open, do not pass keys to Live");
            return false;
        }

    }

    // if we meet no criteria,
    // the original event should
    // be passed to the original caller
    return true;
}

void ActionHandler::handleDoubleRightClick() {
    app_.getWindowManager()->openWindow("ContextMenu");
}

// move to ActionHandler
//				introspect();
//        if (type == kCGEventKeyDown && keyCode == 18 && (CGEventGetFlags(event))) {
//            log_->info("sending new key event");
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


