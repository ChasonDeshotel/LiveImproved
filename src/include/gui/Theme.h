#pragma once

#include <string>
#include <pugixml.hpp>
#include <JuceHeader.h>

class Theme {
public:
    Theme(const std::string& filePath);

    auto getControlTextBack() -> juce::Colour;

    auto getControlForeground() -> juce::Colour;

    auto getColorValue(const std::string& tagName) -> juce::Colour;

private:
    pugi::xml_document doc;

};
