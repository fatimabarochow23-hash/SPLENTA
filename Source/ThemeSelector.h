/*
  ==============================================================================
    ThemeSelector.h (SPLENTA V19.0 - 20251218.01)
    Theme Selector Component: 5 Color Orbs Dock
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class ThemeSelector : public juce::Component
{
public:
    ThemeSelector()
    {
        setInterceptsMouseClicks(true, false);
    }

    ~ThemeSelector() override = default;

    void paint(juce::Graphics& g) override
    {
        const float dockWidth = (float)getWidth();
        const float dockHeight = (float)getHeight();

        // No background - pure color orbs only (macOS traffic light style)

        // Calculate orb properties
        const int padding = 4;
        const int numThemes = 5;
        const float availableWidth = dockWidth - (padding * 2);
        const float orbSpacing = availableWidth / (float)numThemes;
        const float baseOrbRadius = juce::jmin(8.0f, (orbSpacing - 4.0f) * 0.5f);

        // Draw each theme orb
        for (int i = 0; i < numThemes; ++i)
        {
            auto palette = ThemePalette::getPaletteByIndex(i);
            float x = padding + (orbSpacing * i) + (orbSpacing * 0.5f);
            float y = dockHeight * 0.5f;

            bool isSelected = (i == selectedIndex);
            bool isHovered = (i == hoveredIndex);

            float orbRadius = baseOrbRadius;
            float orbAlpha = 0.3f;

            if (isSelected)
            {
                orbRadius = baseOrbRadius + 2.0f;
                orbAlpha = 1.0f;

                // Draw glow effect for selected orb
                g.setColour(palette.glow.withAlpha(0.6f));
                for (int r = 0; r < 8; ++r)
                {
                    float glowRadius = orbRadius + r;
                    float glowAlpha = 0.6f * (1.0f - (r / 8.0f));
                    g.setColour(palette.glow.withAlpha(glowAlpha * 0.3f));
                    g.fillEllipse(x - glowRadius, y - glowRadius, glowRadius * 2.0f, glowRadius * 2.0f);
                }
            }
            else if (isHovered)
            {
                orbAlpha = 1.0f;
            }
            else
            {
                orbRadius = baseOrbRadius - 1.0f;
            }

            // Draw orb
            g.setColour(palette.accent.withAlpha(orbAlpha));
            g.fillEllipse(x - orbRadius, y - orbRadius, orbRadius * 2.0f, orbRadius * 2.0f);
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        hoveredIndex = getOrbIndexAtPosition(e.getPosition());
        repaint();
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hoveredIndex = -1;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        int clickedIndex = getOrbIndexAtPosition(e.getPosition());
        if (clickedIndex >= 0 && clickedIndex < 5)
        {
            selectedIndex = clickedIndex;
            if (onThemeChanged)
                onThemeChanged(selectedIndex);
            repaint();
        }
    }

    void setSelectedIndex(int index)
    {
        if (index >= 0 && index < 5)
        {
            selectedIndex = index;
            repaint();
        }
    }

    int getSelectedIndex() const { return selectedIndex; }

    std::function<void(int)> onThemeChanged;

private:
    int selectedIndex = 0;
    int hoveredIndex = -1;

    int getOrbIndexAtPosition(juce::Point<int> pos)
    {
        const int padding = 4;
        const int numThemes = 5;
        const float availableWidth = (float)getWidth() - (padding * 2);
        const float orbSpacing = availableWidth / (float)numThemes;

        float relativeX = pos.x - padding;
        if (relativeX < 0 || relativeX > availableWidth)
            return -1;

        int index = (int)(relativeX / orbSpacing);
        return juce::jlimit(0, 4, index);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeSelector)
};
