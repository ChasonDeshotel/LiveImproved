#include <ApplicationServices/ApplicationServices.h>
#include <unordered_map>
#include <iostream>
#include <string>

#include "ActionHandler.h"
#include "ApplicationManager.h"
#include "ContextMenu.h"


ActionHandler::ActionHandler(ApplicationManager& appManager)
    : app_(appManager)
    , log_(appManager.getLogHandler())
{
    initializeActionMap();
}

ActionHandler::~ActionHandler() {}

void ActionHandler::init() {
    // should do the mapping / read config or something
}

// Helper function to split string by delimiter
std::pair<std::string, std::string> splitAction(const std::string& action) {
    std::string delimiter = ",";
    size_t pos = action.find(delimiter);
    if (pos == std::string::npos) {
        return {action, ""}; // No delimiter found
    }
    return {action.substr(0, pos), action.substr(pos + delimiter.length())};
}

// need to block all events when 
// app_.getGUISearchBox()->isOpen() = true

// Define action methods
void ActionHandler::foobar() {
    log_->info("foobar method called!");
}

void ActionHandler::sendKeypress(const std::string& key) {
    log_->info("Keypress sent: " + key);
}

// Define a function type for action handlers
using ActionHandlerFunction = std::function<void(const std::string&)>;

// Define a map to link action strings to methods
std::unordered_map<std::string, ActionHandlerFunction> actionMap;

// Initialize the action map
void ActionHandler::initializeActionMap() {
    actionMap["foobar"] = [this](const std::string& args) { this->foobar(); };
    actionMap["keypress"] = [this](const std::string& args) { this->sendKeypress(args); };
}

void ActionHandler::handleDoubleRightClick() {
    ContextMenu* menu = new ContextMenu(nullptr);
    menu->move(QCursor::pos());
    menu->exec();
}

// returns: bool: shouldPassEvent
// -- should the original event be passed
// through to the calling function
// or should the original input be blocked
bool ActionHandler::handleKeyEvent(std::string keyString, CGEventFlags flags, std::string type) {
//    app_.getLogHandler()->info("action handler: Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

    std::unordered_map<std::string, std::string> remap = app_.getConfigManager()->getRemap();

    // Find the remap entry
    auto it = remap.find(keyString);
    if (it != remap.end()) {
        std::string action = it->second;
        // Split the action string into type and arguments
        auto [actionType, args] = splitAction(action);
        auto actionHandler = actionMap.find(actionType);
        if (actionHandler != actionMap.end()) {
            // Call the corresponding method with arguments
            actionHandler->second(args);
        } else {
            log_->info("No handler found for action type: " + actionType);
        }
    } else {
        log_->info("Key not found in remap: " + keyString);
    }


    if (type == "keyDown") {

        if (keyString == "1") {
              log_->info("menu triggered");
              ContextMenu* menu = new ContextMenu(nullptr);
              menu->move(QCursor::pos());
              menu->show();
              return false;
        } else if (keyString == "2") {
              openSearchBox();
              return false;
        } else if (keyString == "3") {
              app_.getIPC()->writeRequest("RELOAD");
              return false;
        } else if (keyString == "4") {
              app_.refreshPluginCache();
              return false;
        } else if (keyString == "5") {
              if (app_.getDragTarget()->isOpen()) {
                  app_.getDragTarget()->closeWindow();
              } else {
                  app_.getDragTarget()->openWindow();
              }
              return false;
        } else if (keyString == "Escape") {
//              if (app_.getGUISearchBox() && app_.getGUISearchBox()->isOpen()) {
                  closeSearchBox();
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
        if (app_.getGUISearchBox()->isOpen()) {
            log_->info("is open, do not pass keys to Live");
            return false;
        }

    }

    // if we meet no criteria,
    // the original event should
    // be passed to the original caller
    return true;
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


bool ActionHandler::openSearchBox() {
    app_.getGUISearchBox()->openSearchBox();
    return false;
}

bool ActionHandler::closeSearchBox() {
    if (app_.getGUISearchBox()->isOpen() && app_.getGUISearchBox()->getSearchTextLength()) {
        app_.getGUISearchBox()->clearSearchText();
    } else {
        app_.getGUISearchBox()->closeSearchBox();
    }
    return false;
}


bool ActionHandler::loadItem(int itemIndex) {
    log_->info("writing request");
    app_.getIPC()->writeRequest("load_item," + std::to_string(itemIndex));
    return false;
}

//bool ActionHandler::onEscapePress() {
//    app_.getLogHandler()->info("Escape pressed");
//    app_.getIPC()->readResponse();
//    return true;
//}

