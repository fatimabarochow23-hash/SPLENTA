/*
  ==============================================================================
    SplitToggleComponent.cpp (SPLENTA V19.0 - 20251218.01)
    Batch 06: Split Dual Toggle (Diagonal Split Button with A/C)
  ==============================================================================
*/

#include "SplitToggleComponent.h"

SplitToggleComponent::SplitToggleComponent(juce::AudioProcessorValueTreeState& apvts)
    : apvts(apvts)
{
    // Initialize with Bronze palette
    palette = ThemePalette::getPaletteByIndex(0);

    // Ensure mouse clicks are intercepted
    setInterceptsMouseClicks(true, false);

    // Get initial states
    auto* agmParam = apvts.getRawParameterValue("AGM_MODE");
    auto* clipParam = apvts.getRawParameterValue("SOFT_CLIP");
    if (agmParam != nullptr)
        agmState = agmParam->load() > 0.5f;
    if (clipParam != nullptr)
        clipState = clipParam->load() > 0.5f;
}

SplitToggleComponent::~SplitToggleComponent()
{
}

void SplitToggleComponent::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
    repaint();
}

void SplitToggleComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();

    // Get current states
    auto* agmParam = apvts.getRawParameterValue("AGM_MODE");
    auto* clipParam = apvts.getRawParameterValue("SOFT_CLIP");
    if (agmParam != nullptr)
        agmState = agmParam->load() > 0.5f;
    if (clipParam != nullptr)
        clipState = clipParam->load() > 0.5f;

    // Use more subtle accent-based colors (less rotation for consistency)
    juce::Colour colorA = palette.accent.withRotatedHue(-0.03f);  // AGM (slightly cooler)
    juce::Colour colorB = palette.accent.withRotatedHue(0.03f);   // Clip (slightly warmer)

    // Create rounded rectangle path for clipping
    const float cornerRadius = 6.0f;
    juce::Path clipPath;
    clipPath.addRoundedRectangle(bounds, cornerRadius);

    // Draw left-top triangle (Auto Gain) with subtle fill
    juce::Path topTriangle;
    topTriangle.startNewSubPath(0, 0);
    topTriangle.lineTo(w, 0);
    topTriangle.lineTo(0, h);
    topTriangle.closeSubPath();

    g.saveState();
    g.reduceClipRegion(clipPath);
    g.setColour(agmState ? colorA.withAlpha(0.2f) : juce::Colours::white.withAlpha(0.03f));
    g.fillPath(topTriangle);
    g.restoreState();

    // Draw right-bottom triangle (Soft Clip) with subtle fill
    juce::Path bottomTriangle;
    bottomTriangle.startNewSubPath(w, 0);
    bottomTriangle.lineTo(w, h);
    bottomTriangle.lineTo(0, h);
    bottomTriangle.closeSubPath();

    g.saveState();
    g.reduceClipRegion(clipPath);
    g.setColour(clipState ? colorB.withAlpha(0.2f) : juce::Colours::white.withAlpha(0.03f));
    g.fillPath(bottomTriangle);
    g.restoreState();

    // Draw diagonal separator line (more subtle)
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawLine(0, h, w, 0, 1.0f);

    // Draw rounded border
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

    // Draw large single letter labels
    g.setFont(juce::FontOptions(28.0f, juce::Font::bold));

    // A for Auto Gain (top-left)
    g.setColour(agmState ? colorA.brighter(0.3f) : juce::Colours::white.withAlpha(0.5f));
    g.drawText("A", bounds.reduced(4.0f).removeFromTop(h * 0.5f), juce::Justification::centredTop);

    // C for Clip (bottom-right)
    g.setColour(clipState ? colorB.brighter(0.3f) : juce::Colours::white.withAlpha(0.5f));
    g.drawText("C", bounds.reduced(4.0f).removeFromBottom(h * 0.5f), juce::Justification::centredBottom);
}

void SplitToggleComponent::resized()
{
}

void SplitToggleComponent::mouseDown(const juce::MouseEvent& event)
{
    float x = event.position.x;
    float y = event.position.y;
    float w = (float)getWidth();
    float h = (float)getHeight();

    // Diagonal line equation: from (0, h) to (w, 0)
    // Line: y = h - (h/w) * x
    // Point is in top-left triangle if: y < h - (h/w) * x

    bool clickedTopLeft = (y < (h - (h / w) * x));

    if (clickedTopLeft)
    {
        // Toggle AGM
        auto* param = apvts.getParameter("AGM_MODE");
        if (param != nullptr)
        {
            bool newState = !agmState;
            param->setValueNotifyingHost(newState ? 1.0f : 0.0f);
            agmState = newState;
        }
    }
    else
    {
        // Toggle Soft Clip
        auto* param = apvts.getParameter("SOFT_CLIP");
        if (param != nullptr)
        {
            bool newState = !clipState;
            param->setValueNotifyingHost(newState ? 1.0f : 0.0f);
            clipState = newState;
        }
    }

    repaint();
}
