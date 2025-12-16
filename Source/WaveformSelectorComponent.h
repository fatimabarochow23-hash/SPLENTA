/*
  ==============================================================================
    WaveformSelectorComponent.h (SPLENTA V18.6 - 20251216.09)
    Batch 06: Custom Waveform Selector (Tray Style)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class WaveformSelectorComponent : public juce::Component
{
public:
    WaveformSelectorComponent(juce::AudioProcessorValueTreeState& apvts);
    ~WaveformSelectorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // Set theme palette for color updates
    void setPalette(const ThemePalette& newPalette);

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;

    ThemePalette palette;
    int selectedIndex = 0;  // 0=Sine, 1=Triangle, 2=Square

    // Draw waveform icons
    void drawSineIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);
    void drawTriangleIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);
    void drawSquareIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformSelectorComponent)
};
