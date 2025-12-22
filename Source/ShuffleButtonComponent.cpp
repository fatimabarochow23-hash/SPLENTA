/*
  ==============================================================================
    ShuffleButtonComponent.cpp (SPLENTA V19.3 - 20251223.01)
    Shuffle/Reset Button - Three Tilted Falling Playing Cards
  ==============================================================================
*/

#include "ShuffleButtonComponent.h"

ShuffleButtonComponent::ShuffleButtonComponent()
{
    palette = ThemePalette::getPaletteByIndex(0);  // Default Bronze theme
}

ShuffleButtonComponent::~ShuffleButtonComponent()
{
}

void ShuffleButtonComponent::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
    repaint();
}

void ShuffleButtonComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Single vertical playing card (portrait orientation)
    float cardWidth = bounds.getWidth() * 0.35f;   // Narrower (21px for 60px button)
    float cardHeight = bounds.getHeight() * 0.75f;  // Taller (18px for 24px button)

    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();

    // Slight tilt for dynamic look
    float angle = 8.0f;

    // Color: accent when hovered, white when not
    juce::Colour cardColor = isHovered ? palette.accent : juce::Colours::white.withAlpha(0.6f);

    g.saveState();

    // Translate to card center
    g.addTransform(juce::AffineTransform::translation(centerX, centerY));
    // Rotate around center
    g.addTransform(juce::AffineTransform::rotation(juce::degreesToRadians(angle)));

    // Draw card rectangle (centered at origin) - vertical orientation
    juce::Rectangle<float> cardRect(-cardWidth/2, -cardHeight/2, cardWidth, cardHeight);

    // Card background
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(cardRect, 1.5f);

    // Card outline
    g.setColour(cardColor);
    g.drawRoundedRectangle(cardRect, 1.5f, 1.5f);

    // Shuffle icon in center (two curved arrows forming a circle)
    float radius = 2.5f;  // Smaller icon for compact card

    // Draw circular arrow path
    juce::Path arrow1, arrow2;

    // Top arrow (clockwise curve)
    arrow1.startNewSubPath(0, -radius);
    arrow1.addCentredArc(0, 0, radius, radius, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    // Arrow head
    arrow1.lineTo(radius * 0.7f, radius * 0.7f);
    arrow1.startNewSubPath(radius, 0);
    arrow1.lineTo(radius * 0.7f, -radius * 0.3f);

    // Bottom arrow (counter-clockwise curve)
    arrow2.startNewSubPath(0, radius);
    arrow2.addCentredArc(0, 0, radius, radius, 0, juce::MathConstants<float>::halfPi, -juce::MathConstants<float>::halfPi, true);
    // Arrow head
    arrow2.lineTo(-radius * 0.7f, -radius * 0.7f);
    arrow2.startNewSubPath(-radius, 0);
    arrow2.lineTo(-radius * 0.7f, radius * 0.3f);

    g.setColour(cardColor.withAlpha(0.7f));
    g.strokePath(arrow1, juce::PathStrokeType(0.8f));
    g.strokePath(arrow2, juce::PathStrokeType(0.8f));

    g.restoreState();
}

void ShuffleButtonComponent::resized()
{
}

void ShuffleButtonComponent::mouseEnter(const juce::MouseEvent& event)
{
    isHovered = true;
    repaint();
}

void ShuffleButtonComponent::mouseExit(const juce::MouseEvent& event)
{
    isHovered = false;
    repaint();
}

void ShuffleButtonComponent::mouseDown(const juce::MouseEvent& event)
{
    if (onShuffle)
        onShuffle();
}
