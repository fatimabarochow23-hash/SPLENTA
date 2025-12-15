/*
  ==============================================================================
    StealthLookAndFeel.h (SPLENTA V18.6 - 20251215.04)
    Custom LookAndFeel for Knob & Fader Interactive Feedback
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class StealthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StealthLookAndFeel();

    void setPalette(const ThemePalette& newPalette);

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos,
                          float minSliderPos,
                          float maxSliderPos,
                          const juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

private:
    ThemePalette palette;

    // Default inactive dot color
    const juce::Colour inactiveDotColor = juce::Colour(168, 162, 158); // #a8a29e
};
