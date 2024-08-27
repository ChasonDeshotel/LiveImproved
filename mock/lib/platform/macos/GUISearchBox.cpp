#include "GUISearchBox.h"
#include "LogHandler.h"
#include <vector>
#include <string>

#include "ApplicationManager.h"
#include "LogHandler.h"
#include "Plugin.h"

GUISearchBox::GUISearchBox(ApplicationManager& appManager)
    : app_(appManager)
    , isOpen_(false), searchText_("") {
    
    LogHandler::getInstance().info("GUISearchBox initialized");
}

GUISearchBox::~GUISearchBox() {
    LogHandler::getInstance().info("GUISearchBox destroyed");
}

bool GUISearchBox::isOpen() const {
    return isOpen_;
}

std::string GUISearchBox::getSearchText() const {
    return searchText_;
}

void GUISearchBox::setSearchText(const std::string text) {
    searchText_ = text;
    LogHandler::getInstance().info("Search text set to: " + searchText_);
}

void GUISearchBox::clearSearchText() {
    searchText_ = "";
    LogHandler::getInstance().info("Search text cleared");
}

void GUISearchBox::openSearchBox() {
    isOpen_ = true;
    LogHandler::getInstance().info("Search box opened");
}

void GUISearchBox::closeSearchBox() {
    isOpen_ = false;
    LogHandler::getInstance().info("Search box closed");
}

void GUISearchBox::setOptions(const std::vector<Plugin>& options) {
    options_ = options;
    LogHandler::getInstance().info("Options set with " + std::to_string(options_.size()) + " items");
}

void GUISearchBox::handlePluginSelected(const Plugin& plugin) {
    LogHandler::getInstance().info("Plugin selected: " + plugin.name);
}
