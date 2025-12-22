/*
  ==============================================================================
    EnvelopeView.h (SPLENTA V19.4 - 20251223.02)
    Frozen Trigger Waveform Display (ShaperBox 3 Style)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Forward declaration
class NewProjectAudioProcessor;

// Keep EnvelopeDataPoint structure for backward compatibility with processor
struct EnvelopeDataPoint
{
    float detector = 0.0f;     // Detector envelope (linear 0.0-1.0+)
    float synthesizer = 0.0f;  // Synthesizer amplitude envelope
    float output = 0.0f;       // Output envelope (computed from final buffer)
};

// Waveform data structure for frozen display
struct WaveformSnapshot
{
    float detectorInput = 0.0f;    // Detector input signal (post-filter)
    float synthOutput = 0.0f;      // Synthesizer output (generated sub)
    float finalOutput = 0.0f;      // Final mixed output
};

//==============================================================================
class EnvelopeView : public juce::Component,
                     public juce::Timer
{
public:
    EnvelopeView(NewProjectAudioProcessor& p);
    ~EnvelopeView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Update waveform snapshot from processor
    void updateFromProcessor();

    // Dynamic theme color integration
    void setThemeColors(juce::Colour accent, juce::Colour panel);

    // Clear waveform display (for shuffle/reset)
    void clearDisplay();

private:
    NewProjectAudioProcessor& processor;

    // Frozen waveform snapshot (captured on trigger)
    // V19.3: Extended to support long amplitude envelopes (up to 0.5s @ 48kHz)
    static constexpr int waveformSize = 24000;
    std::array<WaveformSnapshot, waveformSize> frozenWaveform;
    bool hasValidSnapshot = false;

    // Auto-scale factor (calculated from peak values)
    float autoScaleFactor = 1.0f;

    // Real-time scrolling state
    int triggerScopePos = 0;           // scopeWritePos when triggered
    int64_t triggerTime = 0;           // Timestamp when triggered (in samples)
    bool isScrolling = false;          // Currently scrolling after trigger
    int maxDisplaySamples = 0;         // Maximum samples to display (A_ATT + A_DEC)

    // Trigger detection for freeze mode
    bool lastTriggerState = false;

    // Visual scaling helpers
    inline float mapToVisualY(float sample, float height) const
    {
        // Map bipolar audio sample [-1.0, 1.0] to screen Y [0, height]
        // Used for waveform display (centered at height/2)
        return height * 0.5f * (1.0f - sample);
    }

    inline float mapThresholdToVisualY(float amplitude, float height) const
    {
        // Map unipolar amplitude [0.0, 1.0+] to screen Y [height, 0]
        // Used for threshold/ceiling lines (0 = bottom, 1 = top)
        // Inverted so that higher amplitudes appear higher on screen
        return height * (1.0f - amplitude);
    }

    // Dynamic color scheme (theme-based, updated via setThemeColors)
    juce::Colour backgroundColour { 0xFF0A0A0A };    // Darker background
    juce::Colour detectorColour   { 0x80CC7733 };    // 50% alpha theme color (default bronze)
    juce::Colour outputColour     { 0xFFCC7733 };    // 100% theme color (main layer)
    juce::Colour thresholdColour  { 0xFFFFFFFF };    // White - threshold line
    juce::Colour triggerHighlight { 0x26CC7733 };    // 15% alpha theme color (subtle)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeView)
};
