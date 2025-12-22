/*
  ==============================================================================
    ShuffleButtonComponent.h (SPLENTA V19.3 - 20251223.01)
    Shuffle/Reset Button - Three Tilted Falling Playing Cards
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class ShuffleButtonComponent : public juce::Component
{
public:
    ShuffleButtonComponent();
    ~ShuffleButtonComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setPalette(const ThemePalette& newPalette);

    // Callback when shuffle is triggered
    std::function<void()> onShuffle;

private:
    ThemePalette palette;
    bool isHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShuffleButtonComponent)
};
