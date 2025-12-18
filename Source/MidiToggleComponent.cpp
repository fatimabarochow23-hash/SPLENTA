/*
  ==============================================================================
    MidiToggleComponent.cpp (SPLENTA V19.3 - 20251219.01)
    Minimalist Piano Keyboard Icon Button - MIDI Mode Toggle
  ==============================================================================
*/

#include "MidiToggleComponent.h"

MidiToggleComponent::MidiToggleComponent(juce::AudioProcessorValueTreeState& apvts)
    : apvts(apvts)
{
    palette = ThemePalette::getPaletteByIndex(0);  // Default Bronze theme
}

MidiToggleComponent::~MidiToggleComponent()
{
}

void MidiToggleComponent::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
    repaint();
}

void MidiToggleComponent::drawPianoKeyboardIcon(juce::Graphics& g, juce::Rectangle<int> area, juce::Colour color)
{
    // Ultra-compact piano keyboard icon (8x4 grid)
    // Minimalist design like reference plugins
    std::vector<std::string> icon = {
        "10101010",  // Black keys (top row)
        "11111111",  // White keys (middle)
        "11111111",  // White keys
        "11111111"   // White keys (bottom)
    };

    float pixelWidth = area.getWidth() / 8.0f;
    float pixelHeight = area.getHeight() / 4.0f;

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            if (icon[row][col] == '1')
            {
                juce::Rectangle<float> pixel(area.getX() + col * pixelWidth,
                                             area.getY() + row * pixelHeight,
                                             pixelWidth - 0.5f,
                                             pixelHeight - 0.5f);

                if (row == 0)
                    g.setColour(color.darker(0.6f));  // Black keys
                else
                    g.setColour(color);  // White keys

                g.fillRect(pixel);
            }
        }
    }
}

void MidiToggleComponent::paint(juce::Graphics& g)
{
    midiMode = apvts.getRawParameterValue("MIDI_MODE")->load() > 0.5f;

    auto bounds = getLocalBounds();

    // Minimalist icon design - no button frame, just the icon
    // Icon color changes based on state
    juce::Colour iconColor;
    if (midiMode)
        iconColor = palette.accent;  // Active: full accent color
    else
        iconColor = juce::Colours::white.withAlpha(0.3f);  // Inactive: dim white

    // Draw piano keyboard icon (fill entire bounds)
    drawPianoKeyboardIcon(g, bounds, iconColor);

    // Subtle glow when active
    if (midiMode)
    {
        g.setColour(palette.accent.withAlpha(0.2f));
        g.drawRect(bounds.toFloat().reduced(-1), 1.0f);
    }
}

void MidiToggleComponent::resized()
{
    // Layout is handled in paint()
}

void MidiToggleComponent::mouseDown(const juce::MouseEvent& event)
{
    // Toggle MIDI mode
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("MIDI_MODE")))
    {
        bool newState = !midiMode;
        param->setValueNotifyingHost(newState ? 1.0f : 0.0f);

        // Trigger callback
        if (onMidiModeChanged)
            onMidiModeChanged(newState);
    }
    repaint();
}
