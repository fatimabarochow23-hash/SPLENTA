/*
  ==============================================================================
    PowerButtonComponent.cpp (SPLENTA V19.0 - 20251218.01)
    Bypass Power Button Control
  ==============================================================================
*/

#include "PowerButtonComponent.h"

PowerButtonComponent::PowerButtonComponent(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    // Initialize with default Bronze palette
    palette = ThemePalette::getPaletteByIndex(0);

    // No attachment needed - we'll read the parameter directly
    // and manually toggle it on click

    // Start timer for glow animation
    startTimerHz(60);
}

PowerButtonComponent::~PowerButtonComponent()
{
    stopTimer();
}

void PowerButtonComponent::paint(juce::Graphics& g)
{
    const float width = (float)getWidth();
    const float height = (float)getHeight();
    const float centerX = width * 0.5f;
    const float centerY = height * 0.5f;
    const float radius = juce::jmin(width, height) * 0.4f;

    // Update bypassed state from parameter
    bypassed = apvts.getRawParameterValue("BYPASS")->load() > 0.5f;

    // Color selection based on bypass state
    juce::Colour iconColor;
    if (bypassed)
    {
        // Bypass ON (plugin inactive): Dark gray, low alpha
        iconColor = juce::Colours::grey.withAlpha(0.3f);
    }
    else
    {
        // Bypass OFF (plugin active): Accent color, full brightness
        iconColor = palette.accent.withAlpha(0.9f);
    }

    // Draw outer glow when hovered and active
    if (isHovered && !bypassed)
    {
        juce::ColourGradient glowGradient(
            palette.accent.withAlpha(glowAlpha * 0.4f), centerX, centerY,
            juce::Colours::transparentBlack, centerX + radius * 2.0f, centerY,
            true
        );
        g.setGradientFill(glowGradient);
        g.fillEllipse(centerX - radius * 1.5f, centerY - radius * 1.5f,
                     radius * 3.0f, radius * 3.0f);
    }

    // Draw power icon (particle-based style)
    g.setColour(iconColor);

    // 1. Outer arc (incomplete circle, top gap)
    juce::Path arcPath;
    const float arcStartAngle = juce::MathConstants<float>::pi * 0.75f;  // 135°
    const float arcEndAngle = juce::MathConstants<float>::pi * 2.25f;    // 405° (270° span)

    arcPath.addCentredArc(centerX, centerY, radius, radius,
                          0.0f, arcStartAngle, arcEndAngle, true);

    // Stroke arc with particle dots (simulate using dashed stroke)
    const int arcParticles = 24;
    for (int i = 0; i < arcParticles; ++i)
    {
        float t = i / (float)arcParticles;
        float angle = arcStartAngle + (arcEndAngle - arcStartAngle) * t;
        float px = centerX + std::cos(angle) * radius;
        float py = centerY + std::sin(angle) * radius;

        float dotSize = 1.2f;
        g.fillEllipse(px - dotSize, py - dotSize, dotSize * 2.0f, dotSize * 2.0f);
    }

    // 2. Center vertical line (power symbol stem)
    const float lineHeight = radius * 0.8f;
    juce::Path linePath;
    linePath.startNewSubPath(centerX, centerY - lineHeight);
    linePath.lineTo(centerX, centerY + lineHeight * 0.1f);

    // Draw line with particle dots
    const int lineParticles = 8;
    for (int i = 0; i < lineParticles; ++i)
    {
        float t = i / (float)(lineParticles - 1);
        float py = centerY - lineHeight + lineHeight * 1.1f * t;

        float dotSize = 1.2f;
        g.fillEllipse(centerX - dotSize, py - dotSize, dotSize * 2.0f, dotSize * 2.0f);
    }

    // 3. Status indicator dot (center core)
    float coreSize = bypassed ? 1.5f : 2.0f;
    g.setColour(iconColor.brighter(0.3f));
    g.fillEllipse(centerX - coreSize, centerY - coreSize, coreSize * 2.0f, coreSize * 2.0f);
}

void PowerButtonComponent::resized()
{
    // No dynamic layout needed
}

void PowerButtonComponent::mouseDown(const juce::MouseEvent& event)
{
    // Toggle bypass state
    float currentValue = apvts.getRawParameterValue("BYPASS")->load();
    float newValue = (currentValue > 0.5f) ? 0.0f : 1.0f;
    apvts.getParameter("BYPASS")->setValueNotifyingHost(newValue);

    repaint();
}

void PowerButtonComponent::mouseEnter(const juce::MouseEvent& event)
{
    isHovered = true;
    repaint();
}

void PowerButtonComponent::mouseExit(const juce::MouseEvent& event)
{
    isHovered = false;
    repaint();
}

void PowerButtonComponent::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
    repaint();
}

bool PowerButtonComponent::isBypassed() const
{
    return bypassed;
}

void PowerButtonComponent::timerCallback()
{
    // Animate glow alpha when hovered
    float targetAlpha = (isHovered && !bypassed) ? 1.0f : 0.0f;
    glowAlpha += (targetAlpha - glowAlpha) * 0.15f;

    if (std::abs(glowAlpha - targetAlpha) > 0.01f)
    {
        repaint();
    }
}
