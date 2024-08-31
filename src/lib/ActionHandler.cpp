#include <ApplicationServices/ApplicationServices.h>

#include "ActionHandler.h"
#include "ApplicationManager.h"


ActionHandler::ActionHandler(ApplicationManager& appManager)
    : app_(appManager)
    , log_(appManager.getLogHandler())
{}

ActionHandler::~ActionHandler() {}

void ActionHandler::init() {
    // should do the mapping / read config or something
}

// need to block all events when 
// app_.getGUISearchBox()->isOpen() = true

// returns: bool: shouldPassEvent
// -- should the original event be passed
// through to the calling function
// or should the original input be blocked
bool ActionHandler::handleKeyEvent(CGKeyCode keyCode, CGEventFlags flags, std::string type) {
//    app_.getLogHandler()->info("action handler: Key event: " + type + ", Key code: " + std::to_string(keyCode) + ", Modifiers: " + std::to_string(flags));

    if (type == "keyDown") {

        switch (static_cast<int>(keyCode)) {
//            case 0:  // a
//                return loadItem();
            case 19:
              openSearchBox();
              return false;
            case 20:
              app_.getIPC()->writeRequest("RELOAD");
              return false;
            case 21:
              app_.refreshPluginCache();
              return false;
            case 23: // 5
//              app_.getLogHandler()->info(app_.getPluginCacheAsStr());
              return false;
            case 53:  // escape
//              if (app_.getGUISearchBox() && app_.getGUISearchBox()->isOpen()) {
                  closeSearchBox();
                  return false;
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

            default:
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

