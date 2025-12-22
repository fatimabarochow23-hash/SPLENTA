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

    // Find and cache monospace font name ONCE (performance optimization)
    findAndCacheMonospaceFont();

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

void StealthLookAndFeel::findAndCacheMonospaceFont()
{
    // This is called ONCE in constructor to avoid repeated font scanning
    juce::StringArray fontNames = juce::Font::findAllTypefaceNames();

    // Try to find JetBrains Mono first
    for (const auto& name : fontNames)
    {
        if (name.containsIgnoreCase("JetBrains") && name.containsIgnoreCase("Mono"))
        {
            cachedMonoFontName = name;
            return;
        }
    }

    // Fallback to system monospace fonts
    if (fontNames.contains("Menlo"))
        cachedMonoFontName = "Menlo";
    else if (fontNames.contains("Monaco"))
        cachedMonoFontName = "Monaco";
    else if (fontNames.contains("Consolas"))
        cachedMonoFontName = "Consolas";
    else
        cachedMonoFontName = juce::Font::getDefaultMonospacedFontName();
}

juce::Font StealthLookAndFeel::getMonospaceFont(float height) const
{
    // Use cached font name (no expensive font scanning!)
    return juce::Font(cachedMonoFontName, height, juce::Font::plain);
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

void StealthLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    // Industrial dark background with subtle transparency
    g.fillAll(juce::Colour(0xFF0A0A0A).withAlpha(0.98f));

    // Outer border (theme accent, subtle)
    g.setColour(palette.accent.withAlpha(0.3f));
    g.drawRect(0, 0, width, height, 1);

    // Inner border (darker, for depth)
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(1, 1, width - 2, height - 2, 1);

    // Corner accent marks (L-shaped decorations, industrial style)
    g.setColour(palette.accent.withAlpha(0.4f));
    const int cornerLen = 6;

    // Top-left
    g.drawLine(2, 2, 2 + cornerLen, 2, 1.0f);
    g.drawLine(2, 2, 2, 2 + cornerLen, 1.0f);

    // Top-right
    g.drawLine(width - 2 - cornerLen, 2, width - 2, 2, 1.0f);
    g.drawLine(width - 2, 2, width - 2, 2 + cornerLen, 1.0f);

    // Bottom-left
    g.drawLine(2, height - 2 - cornerLen, 2, height - 2, 1.0f);
    g.drawLine(2, height - 2, 2 + cornerLen, height - 2, 1.0f);

    // Bottom-right
    g.drawLine(width - 2, height - 2 - cornerLen, width - 2, height - 2, 1.0f);
    g.drawLine(width - 2 - cornerLen, height - 2, width - 2, height - 2, 1.0f);
}

void StealthLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                           bool isSeparator, bool isActive,
                                           bool isHighlighted, bool isTicked, bool hasSubMenu,
                                           const juce::String& text, const juce::String& shortcutKeyText,
                                           const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        // Draw separator line (theme accent, subtle)
        auto r = area.reduced(10, 0);
        g.setColour(palette.accent.withAlpha(0.2f));
        g.drawLine(r.getX(), r.getCentreY(), r.getRight(), r.getCentreY(), 1.0f);
    }
    else
    {
        // Check if this is a section header (detect by all-caps or specific pattern)
        bool isSectionHeader = (text == text.toUpperCase() && text.length() < 15);

        if (isSectionHeader)
        {
            // Section Header Styling
            g.setColour(palette.accent);
            g.setFont(getMonospaceFont(10.0f).boldened());
            g.drawText(text, area.reduced(12, 0), juce::Justification::centredLeft);
        }
        else
        {
            // Regular Menu Item
            auto r = area.reduced(8, 0);

            // Highlight background when hovered
            if (isHighlighted && isActive)
            {
                g.setColour(palette.accent.withAlpha(0.15f));
                g.fillRect(area);

                // Left accent bar when highlighted
                g.setColour(palette.accent);
                g.fillRect(area.getX(), area.getY(), 2, area.getHeight());
            }

            // Text color
            juce::Colour itemTextColour = juce::Colours::white.withAlpha(isActive ? 0.85f : 0.4f);
            if (isHighlighted && isActive)
                itemTextColour = palette.accent.brighter(0.3f);

            g.setColour(itemTextColour);
            g.setFont(getMonospaceFont(12.0f));

            // Draw text with left padding
            auto textArea = r.withTrimmedLeft(8);
            g.drawText(text, textArea, juce::Justification::centredLeft);

            // Draw shortcut key (if any) on the right
            if (shortcutKeyText.isNotEmpty())
            {
                g.setFont(getMonospaceFont(10.0f));
                g.setColour(palette.accent.withAlpha(0.5f));
                g.drawText(shortcutKeyText, r.reduced(4, 0), juce::Justification::centredRight);
            }

            // Tick mark for selected items (optional, rarely used in SPLENTA)
            if (isTicked)
            {
                g.setColour(palette.accent);
                g.fillEllipse(r.getX() + 4, r.getCentreY() - 2, 4, 4);
            }
        }
    }
}

juce::Font StealthLookAndFeel::getPopupMenuFont()
{
    return getMonospaceFont(12.0f);
}
