/*
  ==============================================================================
    ABCompareComponent.h (SPLENTA V19.5 - 20251223.01)
    A/B Compare - Top Bar Style with Pyramid Transfer Button
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class NewProjectAudioProcessor;

class ABCompareComponent : public juce::Component, private juce::Timer
{
public:
    ABCompareComponent(NewProjectAudioProcessor& processor);
    ~ABCompareComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setPalette(const ThemePalette& newPalette);

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    NewProjectAudioProcessor& audioProcessor;
    ThemePalette palette;

    // Button bounds (A button, pyramid button, B button)
    juce::Rectangle<float> buttonABounds;
    juce::Rectangle<float> pyramidBounds;
    juce::Rectangle<float> buttonBBounds;

    // Hover states
    bool isHoveringA = false;
    bool isHoveringPyramid = false;
    bool isHoveringB = false;

    // Current state (true = A, false = B)
    bool isStateA = true;

    // Pyramid animation state
    bool isAnimating = false;
    float rotationAngle = 0.0f;           // Current rotation angle
    float targetRotation = 0.0f;          // Target rotation (720° or -720°)
    bool isClockwise = true;              // Rotation direction
    juce::Colour animationColor;          // Color during animation

    // Timer callback for animation
    void timerCallback() override;

    // Update button bounds
    void updateButtonBounds();

    // Draw single button
    void drawButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                   const juce::String& text, bool isActive, bool isHovered);

    // Draw pyramid (3D wireframe tetrahedron)
    void drawPyramid(juce::Graphics& g, juce::Rectangle<float> bounds,
                    float rotation, juce::Colour color, bool isHovered);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ABCompareComponent)
};
