#pragma once

#include <string>
#include <pugixml.hpp>
#include <JuceHeader.h>

class Theme {
public:
    Theme(const std::string& filePath);

    juce::Colour getControlTextBack();

    juce::Colour getControlForeground();

    juce::Colour getColorValue(const std::string& tagName);

private:
    pugi::xml_document doc;

};
