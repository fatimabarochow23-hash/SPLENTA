/*
  ==============================================================================
    RetriggerModeSelector.h (SPLENTA V19.4 - 20251223.03)
    Hard/Soft Retrigger Mode Selector (Sliding Toggle)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"
#include "PluginProcessor.h"

class RetriggerModeSelector : public juce::Component,
                               public juce::Timer
{
public:
    RetriggerModeSelector(NewProjectAudioProcessor& processor);
    ~RetriggerModeSelector() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void timerCallback() override;

    // Set theme palette for color updates
    void setPalette(const ThemePalette& newPalette);

private:
    NewProjectAudioProcessor& audioProcessor;
    ThemePalette palette;

    bool isHardMode = true;  // true=Hard, false=Soft
    float sliderPosition = 0.0f;  // Animated position of sliding indicator (0.0 - 1.0)
    bool isDragging = false;

    // Draw mode icons
    void drawHardIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);
    void drawSoftIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);

    // Update selection from mouse position
    void updateFromMouse(int mouseX);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetriggerModeSelector)
};
