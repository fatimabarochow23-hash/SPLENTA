/*
  ==============================================================================
    MidiToggleComponent.h (SPLENTA V19.3 - 20251219.01)
    Minimalist Piano Keyboard Icon Button - MIDI Mode Toggle
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class MidiToggleComponent : public juce::Component
{
public:
    MidiToggleComponent(juce::AudioProcessorValueTreeState& apvts);
    ~MidiToggleComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // Set theme palette for color updates
    void setPalette(const ThemePalette& newPalette);

    // Set callback for when MIDI mode changes
    std::function<void(bool)> onMidiModeChanged;

private:
    juce::AudioProcessorValueTreeState& apvts;

    ThemePalette palette;
    bool midiMode = false;

    void drawPianoKeyboardIcon(juce::Graphics& g, juce::Rectangle<int> area, juce::Colour color);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiToggleComponent)
};
