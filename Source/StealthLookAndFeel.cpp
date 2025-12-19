/*
  ==============================================================================
    StealthLookAndFeel.cpp (SPLENTA V19.0 - 20251218.01)
    Custom LookAndFeel for Knob & Fader Interactive Feedback + JetBrains Mono
  ==============================================================================
*/

#include "StealthLookAndFeel.h"

StealthLookAndFeel::StealthLookAndFeel()
{
    // Initialize with default Bronze palette
    palette = ThemePalette::getPaletteByIndex(0);

    // Try to set JetBrains Mono as default font
    auto monoFont = getMonospaceFont();
    setDefaultSansSerifTypeface(monoFont.getTypefacePtr());
}

void StealthLookAndFeel::setPalette(const ThemePalette& newPalette)
{
    palette = newPalette;
}

void StealthLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider)
{
    // Determine if slider is active (mouse down or hovering)
    bool isActive = slider.isMouseButtonDown() || slider.isMouseOverOrDragging(true);

    // Calculate center and radius
    float centerX = x + width * 0.5f;
    float centerY = y + height * 0.5f;
    float radius = juce::jmin(width, height) * 0.5f - 2.0f;

    // Draw knob body (dark circle with border)
    g.setColour(palette.panel900.darker(0.3f));
    g.fillEllipse(centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f);

    // Draw border (brighter when active)
    if (isActive)
        g.setColour(palette.accent.withAlpha(0.6f));
    else
        g.setColour(palette.panel900.brighter(0.3f));
    g.drawEllipse(centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // Calculate indicator dot position using trigonometry
    float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    float dotRadius = radius * 0.75f;
    float dotX = centerX + dotRadius * std::cos(angle - juce::MathConstants<float>::halfPi);
    float dotY = centerY + dotRadius * std::sin(angle - juce::MathConstants<float>::halfPi);

    // Indicator dot color
    juce::Colour dotColor = isActive ? palette.accent : inactiveDotColor;

    // Draw glow when active
    if (isActive)
    {
        juce::DropShadow glow(palette.glow.withAlpha(0.6f), 5, juce::Point<int>(0, 0));
        juce::Path dotPath;
        dotPath.addEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);
        glow.drawForPath(g, dotPath);
    }

    // Draw indicator dot
    g.setColour(dotColor);
    g.fillEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);
}

void StealthLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPos,
                                          float minSliderPos,
                                          float maxSliderPos,
                                          const juce::Slider::SliderStyle style,
                                          juce::Slider& slider)
{
    // Only handle vertical sliders
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    bool isActive = slider.isMouseButtonDown() || slider.isMouseOverOrDragging(true);

    // Draw track background
    float trackX = x + width * 0.5f - 2.0f;
    float trackWidth = 4.0f;
    g.setColour(palette.panel900.brighter(0.2f));
    g.fillRoundedRectangle(trackX, (float)y, trackWidth, (float)height, 2.0f);

    // Draw fill (from bottom to sliderPos)
    float fillAlpha = isActive ? 0.8f : 0.5f;
    g.setColour(palette.accent.withAlpha(fillAlpha));
    float fillHeight = (y + height) - sliderPos;
    g.fillRoundedRectangle(trackX, sliderPos, trackWidth, fillHeight, 2.0f);

    // Draw glow when active (optional but recommended)
    if (isActive)
    {
        juce::Path capPath;
        capPath.addRoundedRectangle(trackX - 2.0f, sliderPos - 1.0f, trackWidth + 4.0f, 2.0f, 1.0f);
        juce::DropShadow glow(palette.glow.withAlpha(0.5f), 3, juce::Point<int>(0, 0));
        glow.drawForPath(g, capPath);
    }

    // Draw cap line (always accent)
    g.setColour(palette.accent);
    g.fillRoundedRectangle(trackX - 2.0f, sliderPos - 1.0f, trackWidth + 4.0f, 2.0f, 1.0f);
}

juce::Font StealthLookAndFeel::getMonospaceFont(float height) const
{
    // Try to find JetBrains Mono font on the system
    juce::StringArray fontNames = juce::Font::findAllTypefaceNames();

    for (const auto& name : fontNames)
    {
        if (name.containsIgnoreCase("JetBrains") && name.containsIgnoreCase("Mono"))
        {
            return juce::Font(name, height, juce::Font::plain);
        }
    }

    // Fallback to system monospace fonts
    if (fontNames.contains("Menlo"))
        return juce::Font("Menlo", height, juce::Font::plain);
    if (fontNames.contains("Monaco"))
        return juce::Font("Monaco", height, juce::Font::plain);
    if (fontNames.contains("Consolas"))
        return juce::Font("Consolas", height, juce::Font::plain);

    // Final fallback to JUCE's default monospace
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain);
}

void StealthLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                              juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    // Do nothing - completely transparent button background
    // Icons will be drawn in PluginEditor::paint() instead
}
