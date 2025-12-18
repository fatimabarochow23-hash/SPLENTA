/*
  ==============================================================================
    VirtualKeyboardComponent.h (SPLENTA V19.3 - 20251219.01)
    Virtual MIDI Keyboard with Theme Support + External MIDI Sync
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class VirtualKeyboardComponent : public juce::Component
{
public:
    VirtualKeyboardComponent(juce::MidiKeyboardState& keyboardState);
    ~VirtualKeyboardComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setPalette(const ThemePalette& newPalette);

private:
    juce::MidiKeyboardState& keyboardState;
    juce::MidiKeyboardComponent keyboardComponent;

    ThemePalette palette;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualKeyboardComponent)
};
