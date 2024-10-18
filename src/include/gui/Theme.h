#pragma once

#include <JuceHeader.h>

#include <filesystem>
#include <string>

#include <pugixml.hpp>

class Theme {
public:
    Theme(const std::filesystem::path& filePath);

    auto getControlTextBack() -> juce::Colour;

    auto getControlForeground() -> juce::Colour;

    auto getColorValue(const std::string& tagName) -> juce::Colour;

private:
    pugi::xml_document doc;

};
