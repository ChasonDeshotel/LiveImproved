#include <iostream>
#include <string>
#include <pugixml.hpp>
#include <sstream>

#include <JuceHeader.h>

#include "LogGlobal.h"
#include "Theme.h"

// Helper function to convert hex color to RGB
struct RGB {
    int r, g, b;
    RGB(int r, int g, int b) : r(r), g(g), b(b) {}
};

RGB hexToRGB(const std::string& hex) {
    if (hex[0] == '#') {
        std::stringstream ss;
        ss << std::hex << hex.substr(1);
        unsigned int hexValue;
        ss >> hexValue;
        return RGB((hexValue >> 16) & 0xFF, (hexValue >> 8) & 0xFF, hexValue & 0xFF);
    }
    return RGB(0, 0, 0); // Default black
}

Theme::Theme(const std::filesystem::path& filePath) {
        pugi::xml_parse_result result = doc.load_file(filePath.generic_string().c_str());
        if (!result) {
            logger->error("Failed to load theme file: {}. Reason: {}", filePath.generic_string(), result.description());
        }
    }

auto Theme::getControlTextBack() -> juce::Colour {
    return getColorValue("ControlTextBack");
}

auto Theme::getControlForeground() -> juce::Colour {
    return getColorValue("ControlForeground");
}

auto Theme::getColorValue(const std::string& tagName) -> juce::Colour {
    pugi::xml_node node = doc.child("Ableton").child("Theme").child(tagName.c_str());
    if (node) {
        std::string colorValue = node.attribute("Value").as_string();

        // Add FF (full opacity) after the # if the string starts with #
        if (!colorValue.empty() && colorValue[0] == '#') {
            colorValue = "#FF" + colorValue.substr(1);  // Insert FF for full opacity
        }

        // Convert the color string to a juce::Colour object
        return juce::Colour::fromString(colorValue.c_str());
    }

    // Return a default color if the node or value is not found
    return juce::Colours::transparentBlack;
}
