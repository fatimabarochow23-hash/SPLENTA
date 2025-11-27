/*
  ==============================================================================
    PluginProcessor.h (SPLENTA V17.3 - Final Definitions)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "EnvelopeView.h"  // For EnvelopeDataPoint

class NewProjectAudioProcessor  : public juce::AudioProcessor
{
public:
    NewProjectAudioProcessor();
    ~NewProjectAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    void loadPreset(int presetIndex);
    void setParameterValue(const juce::String& paramID, float value);

    // --- UI Shared Data (Public) ---
    // V17 使用 AudioBuffer 做示波器缓冲
    juce::AudioBuffer<float> scopeBuffer;
    std::atomic<int> scopeWritePos { 0 };

    // Min/Max Peak Detection Buffer for Scope (V18)
    struct PeakPair {
        float minValue = 0.0f;
        float maxValue = 0.0f;
    };
    static constexpr int peakBufferSize = 1024;
    std::array<PeakPair, peakBufferSize> peakBuffer;
    std::atomic<int> peakWritePos { 0 };

    // Envelope Visualization Data (V18.5 - EnvelopeView)
    static constexpr int envelopeBufferSize = 1024;
    std::array<EnvelopeDataPoint, envelopeBufferSize> envelopeBuffer;
    juce::AbstractFifo envelopeFifo { envelopeBufferSize };

    std::atomic<bool> isTriggeredUI { false };
    std::atomic<float> inputRMS { 0.0f };
    std::atomic<float> outputRMS { 0.0f };
    
    // FFT Data
    enum { fftOrder = 11, fftSize = 1 << fftOrder };
    std::array<float, fftSize * 2> fftData;
    std::atomic<bool> nextFFTBlockReady { false };
    
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, fftSize> fifo;
    int fifoIndex = 0;
    void pushNextSampleIntoFifo (float sample);

private:
    double currentSampleRate = 0.0;

    // Peak Detection State
    static constexpr int peakDetectionWindowSize = 256;
    int peakSampleCounter = 0;
    float currentMin = 0.0f;
    float currentMax = 0.0f;

    // Envelope Peak Aggregation (for EnvelopeView)
    static constexpr int envUpdateRate = 64;  // Aggregate every 64 samples
    int envSampleCounter = 0;
    float peakDetector = 0.0f;
    float peakSynthesizer = 0.0f;
    float peakOutput = 0.0f;

    float f_x1 = 0.0f, f_x2 = 0.0f;
    float f_y1 = 0.0f, f_y2 = 0.0f;
    float bf0, bf1, bf2, af1, af2;

    float dry_hp_x1[2] = {0.0f, 0.0f};
    float dry_hp_y1[2] = {0.0f, 0.0f};

    bool isTriggered = false;
    float retriggerTimer = 0.0f;
    float currentPhase = 0.0f;

    float envAmplitude = 0.0f;
    float envPitchValue = 0.0f;
    float envDucking = 0.0f;
    
    // Detector Envelope
    float detectorEnv = 0.0f;

    int pitchState = 0;
    int ampState = 0;
    int duckState = 0;

    float pitchAttackInc = 0.0f; float pitchDecayInc = 0.0f;
    float ampAttackInc = 0.0f;   float ampDecayInc = 0.0f;
    float duckAttackInc = 0.0f;  float duckDecayInc = 0.0f;
    float detectorReleaseCoeff = 0.0f;
    
    juce::LinearSmoothedValue<float> agmGain { 1.0f };

    // --- 参数指针 (必须全部定义) ---
    std::atomic<float>* threshParam = nullptr;
    std::atomic<float>* ceilingParam = nullptr; // NEW
    std::atomic<float>* detReleaseParam = nullptr; // NEW
    std::atomic<float>* auditionParam = nullptr; // NEW

    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* qParam = nullptr;
    std::atomic<float>* waitParam = nullptr;
    
    std::atomic<float>* startFreqParam = nullptr;
    std::atomic<float>* peakFreqParam = nullptr;
    std::atomic<float>* shapeParam = nullptr;
    std::atomic<float>* satParam = nullptr;
    std::atomic<float>* noiseParam = nullptr;
    
    std::atomic<float>* pAttParam = nullptr;
    std::atomic<float>* pDecParam = nullptr;
    std::atomic<float>* aAttParam = nullptr;
    std::atomic<float>* aDecParam = nullptr;
    
    std::atomic<float>* duckParam = nullptr;
    std::atomic<float>* duckAttParam = nullptr;
    std::atomic<float>* duckDecParam = nullptr;
    
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* wetParam = nullptr;
    std::atomic<float>* dryParam = nullptr;
    std::atomic<float>* monitorParam = nullptr; // 保留但可能不用
    std::atomic<float>* agmParam = nullptr;
    std::atomic<float>* clipParam = nullptr;

    void updateFilterCoefficients(float freq, float Q);
    void updateEnvelopeIncrements(float pAtt, float pDec, float aAtt, float aDec, float dAtt, float dDec, float detRel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessor)
};
