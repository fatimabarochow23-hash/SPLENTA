/*
  ==============================================================================
    Theme.h (SPLENTA V19.0 - 20251218.01)
    Theme System: Palette & Enum Definitions (Background Support)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Theme enumeration
enum class ThemeType
{
    Bronze = 0,
    Blue = 1,
    Purple = 2,
    Green = 3,
    Pink = 4
};

// Theme color palette structure
struct ThemePalette
{
    juce::Colour accent;
    juce::Colour glow;
    juce::Colour bg950;
    juce::Colour panel900;

    // Alias for semantic clarity
    juce::Colour background() const { return bg950; }

    // Static factory method to get palette by theme type
    static ThemePalette getPalette(ThemeType theme)
    {
        switch (theme)
        {
            case ThemeType::Bronze:
                return {
                    juce::Colour(0xFFFFB045),  // accent
                    juce::Colour(0xFFFF9000),  // glow
                    juce::Colour(0xFF140C08),  // bg950
                    juce::Colour(0xFF21130D)   // panel900
                };

            case ThemeType::Blue:
                return {
                    juce::Colour(0xFF22D3EE),  // accent
                    juce::Colour(0xFF06B6D4),  // glow
                    juce::Colour(0xFF020617),  // bg950
                    juce::Colour(0xFF0F172A)   // panel900
                };

            case ThemeType::Purple:
                return {
                    juce::Colour(0xFFD8B4FE),  // accent
                    juce::Colour(0xFFC084FC),  // glow
                    juce::Colour(0xFF0F0518),  // bg950
                    juce::Colour(0xFF1E082F)   // panel900
                };

            case ThemeType::Green:
                return {
                    juce::Colour(0xFF4ADE80),  // accent
                    juce::Colour(0xFF22C55E),  // glow
                    juce::Colour(0xFF021205),  // bg950
                    juce::Colour(0xFF062C1B)   // panel900
                };

            case ThemeType::Pink:
                return {
                    juce::Colour(0xFFf0a5c2),  // accent - soft pink #f0a5c2 (图片第一个颜色)
                    juce::Colour(0xFFa75e72),  // glow - mauve pink #a75e72 (图片第二个颜色)
                    juce::Colour(0xFF1A0812),  // bg950 - very dark pink tint
                    juce::Colour(0xFF2D0F1A)   // panel900 - dark pink panel
                };

            default:
                return getPalette(ThemeType::Bronze);
        }
    }

    // Get palette by index
    static ThemePalette getPaletteByIndex(int index)
    {
        if (index < 0 || index > 4)
            index = 0;
        return getPalette(static_cast<ThemeType>(index));
    }
};
