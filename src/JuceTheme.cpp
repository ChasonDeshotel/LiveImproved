#include "JuceTheme.h"
#include "Theme.h"

LimLookAndFeel::LimLookAndFeel(std::function<std::shared_ptr<Theme>()> theme)
    : theme_(std::move(theme))
{
    // Set the default colors
    //
    // SurfaceHighlight -- lighter dark grey
    //
    //
    // SelectionBackground (Value="#b0ddeb")

//    Hex: #b0ddeb (RGB: 176, 221, 235)
//    This is a light blue color, slightly lighter than #A7C6D3, but close in tone.
//
//StandbySelectionBackground (Value="#637e86")
//
//    Hex: #637e86 (RGB: 99, 126, 134)
//    This is a darker, more muted blue/gray, a bit darker than #A7C6D3.
//
//SelectorZoneBackground (Value="#bed6f4")
//
//    Hex: #bed6f4 (RGB: 190, 214, 244)
//    This is a light blue, close to the desired color but a little lighter and more saturated.
    setColour(juce::TextEditor::backgroundColourId, theme_()->getColorValue("SurfaceBackground"));
    setColour(juce::TextEditor::textColourId, theme_()->getColorValue("ControlForeground"));

    setColour(juce::TextEditor::highlightColourId, theme_()->getColorValue("SurfaceBackground"));
    setColour(juce::TextEditor::highlightedTextColourId, theme_()->getColorValue("ControlOnForeground"));
    setColour(juce::TextEditor::outlineColourId, theme_()->getColorValue("Desktop"));
    setColour(juce::TextEditor::focusedOutlineColourId, theme_()->getColorValue("ControlContrastFrame"));
    setColour(juce::TextEditor::shadowColourId, theme_()->getColorValue("ShadowDark"));
    
    setColour(juce::ListBox::backgroundColourId, theme_()->getColorValue("SurfaceBackground"));
    setColour(juce::ListBox::textColourId, theme_()->getColorValue("ControlForeground"));
    setColour(juce::ListBox::outlineColourId, theme_()->getColorValue("ControlBackground"));

    setColour(juce::ScrollBar::backgroundColourId, juce::Colours::darkgrey);
    setColour(juce::ScrollBar::thumbColourId, juce::Colours::lightblue);

    setColour(juce::Slider::thumbColourId, juce::Colours::white);
    setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);

    // More component-specific colors can be added similarly
}

void LimLookAndFeel::drawListBoxOutline(juce::Graphics& g, int width, int height) {
    // Set the outline color and draw the outline
    g.setColour(theme_()->getColorValue("ControlContrastFrame"));
    g.drawRect(0, 0, width, height, 2); // Draw a 2px border around the ListBox
}

    // Override other methods if you want to change default styles
//    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override {
//        return juce::Font("Arial", 16.0f, juce::Font::bold);
//    }

    // Example: Override to create custom slider thumb shape
void LimLookAndFeel::drawLinearSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float minSliderPos, float maxSliderPos,
                                           const juce::Slider::SliderStyle, juce::Slider&) {
    g.setColour(juce::Colours::white);
    g.fillEllipse((float) x, (float) y, (float) width, (float) height);
}
