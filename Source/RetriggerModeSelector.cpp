/*
  ==============================================================================
    RetriggerModeSelector.cpp (SPLENTA V19.4 - 20251223.03)
    Hard/Soft Retrigger Mode Selector (Sliding Toggle)
  ==============================================================================
*/

#include "RetriggerModeSelector.h"

RetriggerModeSelector::RetriggerModeSelector(NewProjectAudioProcessor& processor)
    : audioProcessor(processor)
{
    // Initialize with Bronze palette
    palette = ThemePalette::getPaletteByIndex(0);

    // Ensure mouse clicks are intercepted
    setInterceptsMouseClicks(true, false);

    // Get initial mode from processor
    isHardMode = audioProcessor.retriggerModeHard.load();

    // Initialize slider position to match
    sliderPosition = isHardMode ? 0.0f : 1.0f;

    // Start timer for smooth animation
    startTimerHz(60);
}

RetriggerModeSelector::~RetriggerModeSelector()
{
    stopTimer();
}

void RetriggerModeSelector::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
    repaint();
}

void RetriggerModeSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float segmentWidth = width / 2.0f;

    // Get current mode from processor
    isHardMode = audioProcessor.retriggerModeHard.load();

    // Animate slider position towards target
    float targetPos = isHardMode ? 0.0f : 1.0f;
    sliderPosition += (targetPos - sliderPosition) * 0.3f;

    // Generate analogous colors for Hard/Soft
    juce::Colour hardColor = palette.accent;  // Full brightness for Hard
    juce::Colour softColor = palette.accent.darker(0.3f).withSaturation(0.7f);  // Darker for Soft
    juce::Colour colors[] = {hardColor, softColor};

    // Draw background track with two color zones (always dimmed)
    for (int i = 0; i < 2; ++i)
    {
        float x = i * segmentWidth;
        juce::Rectangle<float> segmentBounds(x, 0, segmentWidth, height);

        g.setColour(colors[i].withAlpha(0.15f));
        g.fillRoundedRectangle(segmentBounds.reduced(2.0f), 4.0f);
    }

    // Draw sliding indicator (transparent overlay that moves)
    float indicatorX = sliderPosition * segmentWidth;
    juce::Rectangle<float> indicatorBounds(indicatorX, 0, segmentWidth, height);

    // Indicator background (bright accent)
    int currentIndex = isHardMode ? 0 : 1;
    g.setColour(colors[currentIndex].withAlpha(0.35f));
    g.fillRoundedRectangle(indicatorBounds.reduced(2.0f), 4.0f);

    // Indicator border (brighter)
    g.setColour(colors[currentIndex].withAlpha(0.6f));
    g.drawRoundedRectangle(indicatorBounds.reduced(2.0f), 4.0f, 1.5f);

    // Draw both mode icons (Hard and Soft)
    for (int i = 0; i < 2; ++i)
    {
        float x = i * segmentWidth;
        juce::Rectangle<float> segmentBounds(x, 0, segmentWidth, height);

        // Icon color (bright when selected, dim otherwise)
        bool isSelected = (i == 0) ? isHardMode : !isHardMode;
        juce::Colour iconColor = isSelected
            ? colors[i].brighter(0.3f)
            : juce::Colours::white.withAlpha(0.3f);

        // Draw mode icon
        auto iconBounds = segmentBounds.reduced(6.0f, 4.0f);
        if (i == 0)
            drawHardIcon(g, iconBounds, iconColor);
        else
            drawSoftIcon(g, iconBounds, iconColor);
    }
}

void RetriggerModeSelector::drawHardIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    // Hard Retrigger: Sharp fade-out envelope (steep decline)
    g.setColour(color);

    juce::Path path;
    float x1 = bounds.getX();
    float x2 = bounds.getRight();
    float yTop = bounds.getY() + bounds.getHeight() * 0.2f;
    float yBottom = bounds.getBottom();

    // Draw steep decline envelope
    path.startNewSubPath(x1, yTop);  // Peak
    path.lineTo(x1 + bounds.getWidth() * 0.3f, yBottom);  // Fast drop (30% width)
    path.lineTo(x2, yBottom);  // Baseline

    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void RetriggerModeSelector::drawSoftIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    // Soft Retrigger: Two gentle fade-out curves (wider spacing, gradual decline)
    g.setColour(color);

    float x1 = bounds.getX();
    float width = bounds.getWidth();
    float yTop = bounds.getY() + bounds.getHeight() * 0.2f;
    float yBottom = bounds.getBottom();

    // First curve (left side, gentle fade-out)
    juce::Path curve1;
    curve1.startNewSubPath(x1, yTop);
    curve1.quadraticTo(x1 + width * 0.3f, yTop + (yBottom - yTop) * 0.4f,
                       x1 + width * 0.45f, yBottom);

    // Second curve (right side, wider spacing)
    juce::Path curve2;
    curve2.startNewSubPath(x1 + width * 0.3f, yTop);
    curve2.quadraticTo(x1 + width * 0.6f, yTop + (yBottom - yTop) * 0.4f,
                       x1 + width * 0.75f, yBottom);

    g.strokePath(curve1, juce::PathStrokeType(1.5f));
    g.strokePath(curve2, juce::PathStrokeType(1.5f));
}

void RetriggerModeSelector::resized()
{
    // No child components to layout
}

void RetriggerModeSelector::mouseDown(const juce::MouseEvent& event)
{
    isDragging = true;
    updateFromMouse(event.x);
}

void RetriggerModeSelector::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging)
    {
        updateFromMouse(event.x);
    }
}

void RetriggerModeSelector::mouseUp(const juce::MouseEvent& event)
{
    isDragging = false;
}

void RetriggerModeSelector::timerCallback()
{
    repaint();
}

void RetriggerModeSelector::updateFromMouse(int mouseX)
{
    float width = (float)getWidth();
    float segmentWidth = width / 2.0f;

    // Determine which half was clicked
    int newIndex = (mouseX < segmentWidth) ? 0 : 1;
    bool newHardMode = (newIndex == 0);

    // Update processor if changed
    if (newHardMode != isHardMode)
    {
        audioProcessor.retriggerModeHard.store(newHardMode);
        isHardMode = newHardMode;
        repaint();
    }
}
