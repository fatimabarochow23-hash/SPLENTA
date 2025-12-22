/*
  ==============================================================================
    VirtualKeyboardComponent.cpp (SPLENTA V19.4 - 20251223.01)
    Virtual MIDI Keyboard with Theme Support + External MIDI Sync
  ==============================================================================
*/

#include "VirtualKeyboardComponent.h"

VirtualKeyboardComponent::VirtualKeyboardComponent(juce::MidiKeyboardState& state)
    : keyboardState(state),
      keyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    palette = ThemePalette::getPaletteByIndex(0);  // Default Bronze theme

    addAndMakeVisible(keyboardComponent);

    // Configure keyboard appearance
    keyboardComponent.setKeyWidth(14.0f);  // Compact size
    keyboardComponent.setLowestVisibleKey(36);  // C2
    keyboardComponent.setOctaveForMiddleC(4);  // Middle C = C4
}

VirtualKeyboardComponent::~VirtualKeyboardComponent()
{
}

void VirtualKeyboardComponent::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;

    // Update keyboard colors
    keyboardComponent.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,
                                 juce::Colours::white.withAlpha(0.9f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::blackNoteColourId,
                                 juce::Colours::black.withAlpha(0.8f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,
                                 juce::Colours::black.withAlpha(0.3f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                                 palette.accent.withAlpha(0.3f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                                 palette.accent.withAlpha(0.8f));

    repaint();
}

void VirtualKeyboardComponent::paint(juce::Graphics& g)
{
    // Dark background panel
    g.fillAll(juce::Colours::black.withAlpha(0.8f));

    // Subtle top border with accent color
    g.setColour(palette.accent.withAlpha(0.3f));
    g.drawLine(0, 0, getWidth(), 0, 2.0f);
}

void VirtualKeyboardComponent::resized()
{
    keyboardComponent.setBounds(getLocalBounds().reduced(4));
}
