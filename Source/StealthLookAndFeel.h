/*
  ==============================================================================
    StealthLookAndFeel.h (SPLENTA V19.0 - 20251218.01)
    Custom LookAndFeel for Knob & Fader Interactive Feedback + JetBrains Mono
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

    // Draw transparent button background (for icon-only buttons)
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    // Get JetBrains Mono font (fallback to monospace if not available)
    juce::Font getMonospaceFont(float height = 14.0f) const;

private:
    ThemePalette palette;

    // Default inactive dot color
    const juce::Colour inactiveDotColor = juce::Colour(168, 162, 158); // #a8a29e
};
