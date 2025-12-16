/*
  ==============================================================================
    WaveformSelectorComponent.cpp (SPLENTA V18.7 - 20251216.10)
    Batch 06: Custom Waveform Selector (Sliding Toggle)
  ==============================================================================
*/

#include "WaveformSelectorComponent.h"

WaveformSelectorComponent::WaveformSelectorComponent(juce::AudioProcessorValueTreeState& apvts)
    : apvts(apvts)
{
    // Initialize with Bronze palette
    palette = ThemePalette::getPaletteByIndex(0);

    // Ensure mouse clicks are intercepted
    setInterceptsMouseClicks(true, false);

    // Get initial value (SHAPE is 0-2: 0=Sine, 1=Triangle, 2=Square)
    auto* param = apvts.getRawParameterValue("SHAPE");
    if (param != nullptr)
        selectedIndex = juce::jlimit(0, 2, juce::roundToInt(param->load()));

    // Initialize slider position to match
    sliderPosition = selectedIndex / 2.0f;

    // Start timer for smooth animation
    startTimerHz(60);
}

WaveformSelectorComponent::~WaveformSelectorComponent()
{
    stopTimer();
}

void WaveformSelectorComponent::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
    repaint();
}

void WaveformSelectorComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float segmentWidth = width / 3.0f;

    // Get current selection (SHAPE is 0-2: 0=Sine, 1=Triangle, 2=Square)
    auto* param = apvts.getRawParameterValue("SHAPE");
    if (param != nullptr)
        selectedIndex = juce::jlimit(0, 2, juce::roundToInt(param->load()));

    // Animate slider position towards target
    float targetPos = selectedIndex / 2.0f;  // 0.0, 0.5, or 1.0
    sliderPosition += (targetPos - sliderPosition) * 0.3f;

    // Generate analogous colors (neighboring hues)
    juce::Colour color1 = palette.accent.withRotatedHue(-0.05f);  // Slightly cooler
    juce::Colour color2 = palette.accent;                         // Center
    juce::Colour color3 = palette.accent.withRotatedHue(0.05f);   // Slightly warmer
    juce::Colour colors[] = {color1, color2, color3};

    // Draw background track with three color zones (always dimmed)
    for (int i = 0; i < 3; ++i)
    {
        float x = i * segmentWidth;
        juce::Rectangle<float> segmentBounds(x, 0, segmentWidth, height);

        g.setColour(colors[i].withAlpha(0.15f));
        g.fillRoundedRectangle(segmentBounds.reduced(2.0f), 4.0f);
    }

    // Draw sliding indicator (transparent overlay that moves)
    float indicatorX = sliderPosition * (width - segmentWidth);
    juce::Rectangle<float> indicatorBounds(indicatorX, 0, segmentWidth, height);

    // Indicator background (bright accent)
    g.setColour(colors[selectedIndex].withAlpha(0.35f));
    g.fillRoundedRectangle(indicatorBounds.reduced(2.0f), 4.0f);

    // Indicator border (brighter)
    g.setColour(colors[selectedIndex].withAlpha(0.6f));
    g.drawRoundedRectangle(indicatorBounds.reduced(2.0f), 4.0f, 2.0f);

    // Draw all three waveform icons
    for (int i = 0; i < 3; ++i)
    {
        float x = i * segmentWidth;
        juce::Rectangle<float> segmentBounds(x, 0, segmentWidth, height);

        // Icon color (bright when selected, dim otherwise)
        bool isSelected = (i == selectedIndex);
        juce::Colour iconColor = isSelected
            ? colors[i].brighter(0.3f)
            : juce::Colours::white.withAlpha(0.3f);

        // Draw waveform icon
        auto iconBounds = segmentBounds.reduced(6.0f, 4.0f);
        switch (i)
        {
            case 0: drawSineIcon(g, iconBounds, iconColor); break;
            case 1: drawTriangleIcon(g, iconBounds, iconColor); break;
            case 2: drawSquareIcon(g, iconBounds, iconColor); break;
        }
    }
}

void WaveformSelectorComponent::resized()
{
}

void WaveformSelectorComponent::mouseDown(const juce::MouseEvent& event)
{
    isDragging = true;
    updateFromMouse(event.x);
}

void WaveformSelectorComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging)
        updateFromMouse(event.x);
}

void WaveformSelectorComponent::mouseUp(const juce::MouseEvent& event)
{
    if (isDragging)
    {
        updateFromMouse(event.x);
        isDragging = false;
    }
}

void WaveformSelectorComponent::updateFromMouse(int mouseX)
{
    int width = getWidth();
    int segmentWidth = width / 3;
    int clickedIndex = mouseX / segmentWidth;

    // Clamp to valid range
    if (clickedIndex < 0) clickedIndex = 0;
    if (clickedIndex > 2) clickedIndex = 2;

    // Only update if changed
    if (clickedIndex != selectedIndex)
    {
        selectedIndex = clickedIndex;

        // Update parameter (SHAPE: 0=Sine, 1=Triangle, 2=Square)
        auto* param = apvts.getParameter("SHAPE");
        if (param != nullptr)
        {
            float normalizedValue = param->convertTo0to1((float)clickedIndex);
            param->setValueNotifyingHost(normalizedValue);
        }
    }
}

void WaveformSelectorComponent::timerCallback()
{
    // Animate slider position and repaint
    repaint();
}

// --- WAVEFORM ICON RENDERERS ---

void WaveformSelectorComponent::drawSineIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    juce::Path sine;
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float cx = bounds.getX();
    float cy = bounds.getY() + height / 2.0f;

    sine.startNewSubPath(cx, cy);
    for (int i = 0; i <= 50; ++i)
    {
        float t = i / 50.0f;
        float x = cx + t * width;
        float y = cy + std::sin(t * juce::MathConstants<float>::twoPi * 1.5f) * height * 0.35f;
        sine.lineTo(x, y);
    }

    g.setColour(color);
    g.strokePath(sine, juce::PathStrokeType(2.0f));
}

void WaveformSelectorComponent::drawTriangleIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    juce::Path triangle;
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float cx = bounds.getX();
    float cy = bounds.getY() + height / 2.0f;

    triangle.startNewSubPath(cx, cy);
    triangle.lineTo(cx + width * 0.25f, cy - height * 0.35f);
    triangle.lineTo(cx + width * 0.75f, cy + height * 0.35f);
    triangle.lineTo(cx + width, cy);

    g.setColour(color);
    g.strokePath(triangle, juce::PathStrokeType(2.0f));
}

void WaveformSelectorComponent::drawSquareIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    juce::Path square;
    float width = bounds.getWidth();
    float height = bounds.getHeight();
    float cx = bounds.getX();
    float cy = bounds.getY() + height / 2.0f;
    float amp = height * 0.35f;

    square.startNewSubPath(cx, cy);
    square.lineTo(cx, cy - amp);
    square.lineTo(cx + width * 0.33f, cy - amp);
    square.lineTo(cx + width * 0.33f, cy + amp);
    square.lineTo(cx + width * 0.66f, cy + amp);
    square.lineTo(cx + width * 0.66f, cy - amp);
    square.lineTo(cx + width, cy - amp);
    square.lineTo(cx + width, cy);

    g.setColour(color);
    g.strokePath(square, juce::PathStrokeType(2.0f));
}
