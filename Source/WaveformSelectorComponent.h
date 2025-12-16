/*
  ==============================================================================
    WaveformSelectorComponent.h (SPLENTA V18.7 - 20251216.10)
    Batch 06: Custom Waveform Selector (Sliding Toggle)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class WaveformSelectorComponent : public juce::Component,
                                  public juce::Timer
{
public:
    WaveformSelectorComponent(juce::AudioProcessorValueTreeState& apvts);
    ~WaveformSelectorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void timerCallback() override;

    // Set theme palette for color updates
    void setPalette(const ThemePalette& newPalette);

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;

    ThemePalette palette;
    int selectedIndex = 0;  // 0=Sine, 1=Triangle, 2=Square
    float sliderPosition = 0.0f;  // Animated position of sliding indicator (0.0 - 1.0)
    bool isDragging = false;

    // Draw waveform icons
    void drawSineIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);
    void drawTriangleIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);
    void drawSquareIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);

    // Update selection from mouse position
    void updateFromMouse(int mouseX);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformSelectorComponent)
};
