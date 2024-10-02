#include <JuceHeader.h>

class Theme;

class LimLookAndFeel : public juce::LookAndFeel_V4 {
public:
    LimLookAndFeel(
        std::function<std::shared_ptr<Theme>()> theme
    );

    void drawLinearSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float minSliderPos, float maxSliderPos,
                                           const juce::Slider::SliderStyle, juce::Slider&) override;

    void drawListBoxOutline(juce::Graphics& g, int width, int height);

private:
    std::function<std::shared_ptr<Theme>()> theme_;
};
