/*
  ==============================================================================
    EnergyTopologyComponent.h (SPLENTA V19.5 - 20251224.01)
    Energy Topology: Wormhole + Figure-8 Lemniscate Implementation
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

    // Trigger effect for particle scatter/gather
    void setTriggerState(bool isTriggered);

    // Bypass and Saturation control (V19.0)
    void setBypassState(bool isBypassed);
    void setSaturation(float satValue);  // 0.0f to 100.0f

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
    static constexpr int numParticles = 1200;  // Increased for wormhole (needs both sides)
    std::vector<Particle> particles;

    // Animation state
    float time = 0.0f;
    float intensity = 50.0f;     // 0-100
    ThemePalette palette;

    // Bypass and Saturation state (V19.0)
    bool isBypassed = false;
    float saturation = 25.0f;    // 0-100 (default from SATURATION parameter)

    // Trigger scatter effect
    bool lastTriggerState = false;
    float scatterAmount = 0.0f;      // 0.0 = stable, 1.0 = fully scattered
    std::vector<juce::Point<float>> particleOffsets; // Scatter offsets for each particle

    // Healing light beams for Pink theme (recovery effect)
    struct LightBeam {
        float x, z;          // Position around heart base
        float height;        // Current height (0.0 = bottom, 1.0 = top)
        float speed;         // Rise speed
        float alpha;         // Opacity
        float phase;         // Animation phase offset
    };
    std::vector<LightBeam> healingBeams;
    float beamSpawnTimer = 0.0f;

    // Heart beat timing
    float lastBeatTime = 0.0f;
    static constexpr float beatInterval = 1.8f; // 1.8 seconds per beat (~33 BPM)

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

    // Pink theme now renders anatomical heart (no helper needed)

    // Helper: Convert accent color to RGBA string equivalent
    juce::Colour getColorWithAlpha(float alpha);

    // Helper: Calculate particle size multiplier based on bypass/SAT state
    float getParticleSizeMultiplier() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnergyTopologyComponent)
};
