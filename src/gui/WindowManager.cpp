#include "WindowManager.h"

#include "LogGlobal.h"

#include "IEventHandler.h"
#include "IEventHandler.h"

#include "IWindow.h"

#include "ContextMenu.h"
#include "SearchBox.h"
#include "Theme.h"

WindowManager::WindowManager(
                             std::function<std::shared_ptr<IPluginManager>()> pluginManager
                             , std::function<std::shared_ptr<IEventHandler>()> eventHandler
                             , std::function<std::shared_ptr<IActionHandler>()> actionHandler
                             , std::function<std::shared_ptr<WindowManager>()> windowManager
                             , std::function<std::shared_ptr<Theme>()> theme
                             , std::function<std::shared_ptr<LimLookAndFeel>()> limLookAndFeel
                             , std::function<std::shared_ptr<ConfigMenu>()> configMenu
                             )
    : pluginManager_(std::move(pluginManager))
    , eventHandler_(std::move(eventHandler))
    , actionHandler_(std::move(actionHandler))
    , windowManager_(std::move(windowManager))
    , theme_(std::move(theme))
    , limLookAndFeel_(std::move(limLookAndFeel))
    , configMenu_(std::move(configMenu))
{}

// Factory function to create window instances dynamically based on the name
auto WindowManager::createWindowInstance(const std::string& windowName) -> std::unique_ptr<IWindow> {
    if (windowName == "ContextMenu") {
        // TODO
        return std::make_unique<ContextMenu>(configMenu_, actionHandler_, windowManager_);
    } else if (windowName == "SearchBox") {
        return std::make_unique<SearchBox>(pluginManager_, eventHandler_, actionHandler_, windowManager_, theme_, limLookAndFeel_);
    }
    return nullptr;
}

void WindowManager::registerWindow(const std::string& windowName, std::function<void()> callback) {
	if (windows_.find(windowName) == windows_.end()) {
		std::shared_ptr<IWindow> windowInstance = createWindowInstance(windowName);
		if (windowInstance != nullptr) {
			windows_[windowName] = {.window = windowInstance, .callback = callback};
			windowStates_[windowName] = false;
		} else {
			throw std::runtime_error("Window class not found for: " + windowName);
		}
	}
}

auto WindowManager::getWindowHandle(const std::string& windowName) const -> void* {
    auto it = windows_.find(windowName);

    if (it != windows_.end()) {
        return it->second.window->getWindowHandle();
    } else {
        throw std::runtime_error("Window not found for: " + windowName);
    }
}

void WindowManager::openWindow(const std::string& windowName) {
    logger->debug("WM open called");

    auto it = windows_.find(windowName);

    // register the window if it doesn't yet exist
    if (it == windows_.end()) {
        logger->debug("window not registered, registering...");
        registerWindow(windowName, []() {});
        it = windows_.find(windowName); // Re-fetch the iterator
    }

    if (it->second.callback) {
        logger->debug("WM calling callback");
        it->second.callback();
    }

    logger->debug("Current window state for " + windowName + ": " + std::to_string(windowStates_[windowName]));
    if (!windowStates_[windowName]) {
        // TODO: making some assumptions by setting this to true before
        // calling `open()`, but the context menu is blocking so...
        windowStates_[windowName] = true;
        logger->debug("WM calling open");
        it->second.window->open();
    }
}

void WindowManager::closeWindow(const std::string& windowName) {
    logger->debug("WM close called");

    auto it = windows_.find(windowName);
    if (it != windows_.end() && windowStates_[windowName]) {
        it->second.window->close();
    }
    windowStates_[windowName] = false;
    logger->debug("updated window state to close");
    logger->debug("close - Current window state for " + windowName + ": " + std::to_string(windowStates_[windowName]));
    eventHandler_()->focusLive();
}

void WindowManager::toggleWindow(const std::string& windowName) {
    logger->debug("WM close called");

    auto it = windows_.find(windowName);

    // window isn't registered -- we'll create it (with open)
    if (it == windows_.end()) {
        logger->error("Window not found: " + windowName);
        openWindow(windowName);
        return;
    }

    bool isOpen = windowStates_[windowName];

    if (isOpen) {
        closeWindow(windowName);
        windowStates_[windowName] = !isOpen;
    } else {
        closeWindow(windowName);
        windowStates_[windowName] = !isOpen;
    }

    logger->debug("toggle - current window state for " + windowName + ": " + std::to_string(windowStates_[windowName]));
}

auto WindowManager::isWindowOpen(const std::string& windowName) const -> bool {
    auto it = windowStates_.find(windowName);
    return (it != windowStates_.end() && it->second);
}

// TODO
// closeAllWindows
// isWindowFocused
// focusWindow
// closeFocusedWindow
