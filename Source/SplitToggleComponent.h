/*
  ==============================================================================
    SplitToggleComponent.h (SPLENTA V18.7 - 20251216.10)
    Batch 06: Split Dual Toggle (Diagonal Split Button with A/C)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class SplitToggleComponent : public juce::Component
{
public:
    SplitToggleComponent(juce::AudioProcessorValueTreeState& apvts);
    ~SplitToggleComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // Set theme palette for color updates
    void setPalette(const ThemePalette& newPalette);

private:
    juce::AudioProcessorValueTreeState& apvts;

    ThemePalette palette;
    bool agmState = false;
    bool clipState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitToggleComponent)
};
