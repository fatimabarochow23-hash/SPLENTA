/*
  ==============================================================================
    SplitToggleComponent.cpp (SPLENTA V18.6 - 20251216.09)
    Batch 06: Split Dual Toggle (Diagonal Split Button)
  ==============================================================================
*/

#include "SplitToggleComponent.h"

SplitToggleComponent::SplitToggleComponent(juce::AudioProcessorValueTreeState& apvts)
    : apvts(apvts)
{
    // Initialize with Bronze palette
    palette = ThemePalette::getPaletteByIndex(0);

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

    // Generate two analogous colors
    juce::Colour colorA = palette.accent.withRotatedHue(-0.08f);  // AGM (cooler)
    juce::Colour colorB = palette.accent.withRotatedHue(0.08f);   // Clip (warmer)

    // Draw left-top triangle (Auto Gain)
    juce::Path topTriangle;
    topTriangle.startNewSubPath(0, 0);
    topTriangle.lineTo(w, 0);
    topTriangle.lineTo(0, h);
    topTriangle.closeSubPath();

    g.setColour(agmState ? colorA.withAlpha(0.4f) : juce::Colours::white.withAlpha(0.05f));
    g.fillPath(topTriangle);

    // Draw right-bottom triangle (Soft Clip)
    juce::Path bottomTriangle;
    bottomTriangle.startNewSubPath(w, 0);
    bottomTriangle.lineTo(w, h);
    bottomTriangle.lineTo(0, h);
    bottomTriangle.closeSubPath();

    g.setColour(clipState ? colorB.withAlpha(0.4f) : juce::Colours::white.withAlpha(0.05f));
    g.fillPath(bottomTriangle);

    // Draw diagonal separator line
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawLine(0, h, w, 0, 1.5f);

    // Draw border
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(bounds, 1.0f);

    // Draw labels
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));

    // AGM label (top-left)
    g.setColour(agmState ? colorA : juce::Colours::white.withAlpha(0.4f));
    g.drawText("AGM", bounds.reduced(4.0f).removeFromTop(h * 0.4f), juce::Justification::centredTop);

    // CLIP label (bottom-right)
    g.setColour(clipState ? colorB : juce::Colours::white.withAlpha(0.4f));
    g.drawText("CLIP", bounds.reduced(4.0f).removeFromBottom(h * 0.4f), juce::Justification::centredBottom);
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
