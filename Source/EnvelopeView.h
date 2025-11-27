/*
  ==============================================================================
    EnvelopeView.h (SPLENTA V18.5 - 20251127.01)
    Dynamic Envelope Network Visualization Component
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Forward declaration
class NewProjectAudioProcessor;

// Data structure for three envelope signals (linear amplitude values)
struct EnvelopeDataPoint
{
    float detector = 0.0f;     // Detector envelope (linear 0.0-1.0+)
    float synthesizer = 0.0f;  // Synthesizer amplitude envelope
    float output = 0.0f;       // Output envelope (computed from final buffer)
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

    // Read new data from processor's FIFO and update history
    void updateFromProcessor();

    // Dynamic theme color integration
    void setThemeColors(juce::Colour accent, juce::Colour panel);

private:
    NewProjectAudioProcessor& processor;

    // Fixed history buffer (512 points)
    static constexpr int historySize = 512;
    std::array<EnvelopeDataPoint, historySize> historyBuffer;

    // Logarithmic scaling constants for ~60dB dynamic range
    static constexpr float logScaleK = 1000.0f;
    static inline const float invLog1pk = 1.0f / std::log(1.0f + logScaleK);

    // Efficient logarithmic mapping function
    inline float mapToLogScale(float linearValue) const
    {
        // Clamp to avoid negative values
        linearValue = juce::jmax(0.0f, linearValue);
        return std::log(1.0f + logScaleK * linearValue) * invLog1pk;
    }

    // Dynamic color scheme (updated via setThemeColors)
    juce::Colour backgroundColour { 0xFF1A0E0A };
    juce::Colour detectorColour   { 0xFFCC7733 };
    juce::Colour synthColour      { 0xFFFF8C42 };
    juce::Colour outputColour     { 0xFFFFD66B };
    juce::Colour referenceColour  { 0xFFA0E6E3 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeView)
};
