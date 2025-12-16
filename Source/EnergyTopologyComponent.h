/*
  ==============================================================================
    EnergyTopologyComponent.h (SPLENTA V18.6 - 20251216.07)
    Energy Topology: Mobius Visualizer (Cartesian Trek Update)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class EnergyTopologyComponent : public juce::Component,
                                 private juce::Timer
{
public:
    EnergyTopologyComponent();
    ~EnergyTopologyComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    // Theme integration
    void setPalette(const ThemePalette& newPalette);
    void setIntensity(float intensityValue); // 0.0f to 100.0f

private:
    void timerCallback() override;

    // Particle structure (matches Web version)
    struct Particle {
        float x, y, z;           // Generic coords
        float vx, vy;            // Velocities (unused in some themes)
        float angle;             // Parameter for Mobius/Sakura
        float radius;            // Orbital radius
        float offset;            // Phase offset
        float speed;             // Animation speed
        float life;              // Life cycle (0.0-1.0)
    };

    // Particle system
    static constexpr int numParticles = 400;
    std::vector<Particle> particles;

    // Animation state
    float time = 0.0f;
    float intensity = 50.0f;     // 0-100
    ThemePalette palette;

    // Theme renderers
    void drawMobius(juce::Graphics& g, float width, float height, float cx, float cy);
    void drawWaves(juce::Graphics& g, float width, float height, float cx, float cy);
    void drawMoon(juce::Graphics& g, float width, float height, float cx, float cy);
    void drawNetwork(juce::Graphics& g, float width, float height, float cx, float cy);
    void drawCartesian(juce::Graphics& g, float width, float height, float cx, float cy);

    // 3D Projection helper
    struct Projection3D {
        float x, y, scale, z;
    };
    Projection3D project3D(float x, float y, float z, float cx, float cy);

    // Helper: Convert accent color to RGBA string equivalent
    juce::Colour getColorWithAlpha(float alpha);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnergyTopologyComponent)
};
