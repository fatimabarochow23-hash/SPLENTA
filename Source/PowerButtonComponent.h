/*
  ==============================================================================
    PowerButtonComponent.h (SPLENTA V19.0 - 20251218.01)
    Bypass Power Button Control
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class PowerButtonComponent : public juce::Component,
                              private juce::Timer
{
public:
    PowerButtonComponent(juce::AudioProcessorValueTreeState& apvts);
    ~PowerButtonComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void setPalette(const ThemePalette& newPalette);

    // Get bypass state for external queries
    bool isBypassed() const;

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;

    ThemePalette palette;

    bool bypassed = false;
    bool isHovered = false;
    float glowAlpha = 0.0f;  // Animated glow effect

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PowerButtonComponent)
};
