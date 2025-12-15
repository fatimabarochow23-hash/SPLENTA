/*
  ==============================================================================
    PluginProcessor.cpp (SPLENTA V18.6 - 20251212.01)
    Temporal Window Optimization & Dynamic Scaling
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       forwardFFT (fftOrder),
       window (fftSize, juce::dsp::WindowingFunction<float>::hann)
#endif
{
    apvts.reset (new juce::AudioProcessorValueTreeState (*this, nullptr, "Parameters", createParameterLayout()));

    threshParam   = apvts->getRawParameterValue("THRESHOLD");
    ceilingParam  = apvts->getRawParameterValue("CEILING");
    detReleaseParam = apvts->getRawParameterValue("DET_REL");
    auditionParam = apvts->getRawParameterValue("AUDITION");
    
    freqParam     = apvts->getRawParameterValue("F_FREQ");
    qParam        = apvts->getRawParameterValue("F_Q");
    waitParam     = apvts->getRawParameterValue("WAIT_MS");
    
    startFreqParam= apvts->getRawParameterValue("START_FREQ");
    peakFreqParam = apvts->getRawParameterValue("PEAK_FREQ");
    shapeParam    = apvts->getRawParameterValue("SHAPE");
    satParam      = apvts->getRawParameterValue("SATURATION");
    noiseParam    = apvts->getRawParameterValue("NOISE_MIX");
    
    pAttParam     = apvts->getRawParameterValue("P_ATT");
    pDecParam     = apvts->getRawParameterValue("P_DEC");
    aAttParam     = apvts->getRawParameterValue("A_ATT");
    aDecParam     = apvts->getRawParameterValue("A_DEC");
    
    duckParam     = apvts->getRawParameterValue("DUCKING");
    duckAttParam  = apvts->getRawParameterValue("D_ATT");
    duckDecParam  = apvts->getRawParameterValue("D_DEC");
    
    mixParam      = apvts->getRawParameterValue("MIX");
    wetParam      = apvts->getRawParameterValue("WET_GAIN");
    dryParam      = apvts->getRawParameterValue("DRY_MIX");
    agmParam      = apvts->getRawParameterValue("AGM_MODE");
    clipParam     = apvts->getRawParameterValue("SOFT_CLIP");

    // Scope Buffer Init
    scopeBuffer.setSize(1, 2048);
    scopeBuffer.clear();

    // Peak Buffer Init
    for (auto& peak : peakBuffer) {
        peak.minValue = 0.0f;
        peak.maxValue = 0.0f;
    }
    peakWritePos = 0;
    peakSampleCounter = 0;
    currentMin = 0.0f;
    currentMax = 0.0f;

    // Envelope Buffer Init
    for (auto& point : envelopeBuffer) {
        point.detector = 0.0f;
        point.synthesizer = 0.0f;
        point.output = 0.0f;
    }
    envSampleCounter = 0;
    peakDetector = 0.0f;
    peakSynthesizer = 0.0f;
    peakOutput = 0.0f;

    std::fill(fftData.begin(), fftData.end(), 0.0f);
}

NewProjectAudioProcessor::~NewProjectAudioProcessor() {}

const juce::String NewProjectAudioProcessor::getName() const { return JucePlugin_Name; }
bool NewProjectAudioProcessor::acceptsMidi() const { return false; }
bool NewProjectAudioProcessor::producesMidi() const { return false; }
bool NewProjectAudioProcessor::isMidiEffect() const { return false; }
double NewProjectAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int NewProjectAudioProcessor::getNumPrograms() { return 1; }
int NewProjectAudioProcessor::getCurrentProgram() { return 0; }
void NewProjectAudioProcessor::setCurrentProgram (int index) {}
const juce::String NewProjectAudioProcessor::getProgramName (int index) { return {}; }
void NewProjectAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    atomicSampleRate.store(sampleRate);  // Store atomic sample rate
    updateFilterCoefficients(*freqParam, *qParam);
    updateEnvelopeIncrements(*pAttParam, *pDecParam, *aAttParam, *aDecParam, *duckAttParam, *duckDecParam, *detReleaseParam);
    
    isTriggered = false;
    retriggerTimer = 0.0f;
    currentPhase = 0.0f;
    detectorEnv = 0.0f;

    // Reset Peak Detection
    peakSampleCounter = 0;
    currentMin = 0.0f;
    currentMax = 0.0f;

    // Reset Envelope Aggregation
    envSampleCounter = 0;
    peakDetector = 0.0f;
    peakSynthesizer = 0.0f;
    peakOutput = 0.0f;

    f_x1 = 0.0f; f_x2 = 0.0f; f_y1 = 0.0f; f_y2 = 0.0f;
    dry_hp_x1[0] = 0.0f; dry_hp_y1[0] = 0.0f;
    dry_hp_x1[1] = 0.0f; dry_hp_y1[1] = 0.0f;
    
    agmGain.reset(sampleRate, 0.05);
    agmGain.setCurrentAndTargetValue(1.0f);
}

void NewProjectAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

void NewProjectAudioProcessor::pushNextSampleIntoFifo (float sample)
{
    if (fifoIndex == fftSize) {
        if (!nextFFTBlockReady) {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}

void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();
    
    float currentInputRMS = buffer.getRMSLevel(0, 0, numSamples);
    inputRMS = currentInputRMS;

    updateFilterCoefficients(*freqParam, *qParam);
    updateEnvelopeIncrements(*pAttParam, *pDecParam, *aAttParam, *aDecParam, *duckAttParam, *duckDecParam, *detReleaseParam);
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    bool isAuditioning = auditionParam->load() > 0.5f;
    auto* scopeWrite = scopeBuffer.getWritePointer(0);
    int scopeSize = scopeBuffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float inputL = buffer.getReadPointer(0)[sample];
        float inputMono = inputL;
        if (totalNumInputChannels > 1)
            inputMono = (inputL + buffer.getReadPointer(1)[sample]) * 0.5f;

        // 1. Detector Filter
        float tf_out = bf0 * inputMono + bf1 * f_x1 + bf2 * f_x2 - af1 * f_y1 - af2 * f_y2;
        f_x2 = f_x1; f_x1 = inputMono;
        f_y2 = f_y1; f_y1 = tf_out;
        
        float absIn = std::abs(tf_out);
        if (absIn > detectorEnv) detectorEnv = absIn;
        else detectorEnv = detectorEnv * detectorReleaseCoeff + absIn * (1.0f - detectorReleaseCoeff);

        // Legacy Scope Buffer (keep for compatibility)
        scopeWrite[scopeWritePos] = tf_out;
        scopeWritePos = (scopeWritePos + 1) % scopeSize;

        // Peak Detection for Professional Scope Display
        if (peakSampleCounter == 0) {
            // Initialize new peak window
            currentMin = tf_out;
            currentMax = tf_out;
        } else {
            // Update min/max
            if (tf_out < currentMin) currentMin = tf_out;
            if (tf_out > currentMax) currentMax = tf_out;
        }

        peakSampleCounter++;
        if (peakSampleCounter >= peakDetectionWindowSize) {
            // Store the min/max pair
            int writeIndex = peakWritePos.load();
            peakBuffer[writeIndex].minValue = currentMin;
            peakBuffer[writeIndex].maxValue = currentMax;
            peakWritePos = (writeIndex + 1) % peakBufferSize;

            // Reset for next window
            peakSampleCounter = 0;
            currentMin = 0.0f;
            currentMax = 0.0f;
        }

        // 2. Trigger (Window)
        float threshLin = juce::Decibels::decibelsToGain(threshParam->load());
        float ceilingLin = juce::Decibels::decibelsToGain(ceilingParam->load());
        bool inRange = (detectorEnv > threshLin) && (detectorEnv < ceilingLin || ceilingLin >= 0.99f); // Ceiling 0dB = off roughly

        if (inRange && retriggerTimer <= 0.0f && detectorEnv > threshLin) // Ensure above thresh
        {
            isTriggered = true;
            retriggerTimer = waitParam->load() * (currentSampleRate / 1000.0f);
            envAmplitude = 0.0f; envPitchValue = 0.0f; envDucking = 0.0f;
            pitchState = 0; ampState = 0; duckState = 0;
            currentPhase = 1.5707f;
        }
        if (retriggerTimer > 0.0f) retriggerTimer -= 1.0f;
        isTriggeredUI = isTriggered;

        // 3. Envelopes
        if (isTriggered)
        {
            if (pitchState == 0) { envPitchValue += pitchAttackInc; if(envPitchValue>=1.0f) {envPitchValue=1.0f; pitchState=1;} }
            else { envPitchValue -= pitchDecayInc; if(envPitchValue<=0.0f) envPitchValue=0.0f; }
            
            if (ampState == 0) { envAmplitude += ampAttackInc; if(envAmplitude>=1.0f) {envAmplitude=1.0f; ampState=1;} }
            else { envAmplitude -= ampDecayInc; if(envAmplitude<=0.0f) {envAmplitude=0.0f; isTriggered=false;} }

            if (duckState == 0) { envDucking += duckAttackInc; if(envDucking>=1.0f) {envDucking=1.0f; duckState=1;} }
            else { envDucking -= duckDecayInc; if(envDucking<=0.0f) envDucking=0.0f; }
        }
        
        // 4. Synthesis
        float startFreq = startFreqParam->load();
        float peakFreq  = peakFreqParam->load();
        float oscShape  = shapeParam->load();
        float driveAmount = satParam->load() / 100.0f;
        float noiseAmount = noiseParam->load() / 100.0f;
        
        float currentFreq = startFreq + (peakFreq - startFreq) * envPitchValue;
        currentPhase += (currentFreq / (float)currentSampleRate) * juce::MathConstants<float>::twoPi;
        if (currentPhase > juce::MathConstants<float>::twoPi) currentPhase -= juce::MathConstants<float>::twoPi;

        float oscRaw = 0.0f;
        if (oscShape < 0.5f) oscRaw = std::sin(currentPhase);
        else if (oscShape < 1.5f) oscRaw = 1.0f - 2.0f * std::abs((currentPhase / juce::MathConstants<float>::pi) - 1.0f);
        else oscRaw = (currentPhase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;

        float noiseRaw = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * noiseAmount;
        float driveSig = (oscRaw + noiseRaw) * (1.0f + driveAmount * 3.0f);
        float oscProc = driveSig / (1.0f + std::abs(driveSig));
        
        float finalWet = oscProc * envAmplitude * juce::Decibels::decibelsToGain(wetParam->load());

        // 5. Spectral Ducking
        float duckDB = duckParam->load();
        float duckAmount = 1.0f - juce::Decibels::decibelsToGain(duckDB);
        float currentDuckGain = 1.0f - (envDucking * duckAmount);

        float dryHPFreq = 20.0f + (envDucking * 300.0f);
        float hp_w0 = 2.0f * juce::MathConstants<float>::pi * dryHPFreq / (float)currentSampleRate;
        float hp_x = std::exp(-hp_w0);
        float hp_a1 = hp_x;
        float hp_b0 = 0.5f * (1.0f + hp_x);
        float hp_b1 = -hp_b0;

        float mixPct = mixParam->load() / 100.0f;
        bool useSoftClip = clipParam->load() > 0.5f;

        // 6. Output
        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
             auto* channelData = buffer.getWritePointer (ch);
             float drySig = channelData[sample];

             if (ch == 0) pushNextSampleIntoFifo(finalWet + drySig); // FFT

             if (isAuditioning)
             {
                 channelData[sample] = tf_out;
             }
             else
             {
                 float filteredDry = hp_b0 * drySig + hp_b1 * dry_hp_x1[ch] + hp_a1 * dry_hp_y1[ch];
                 dry_hp_x1[ch] = drySig;
                 dry_hp_y1[ch] = filteredDry;

                 // Additive Mix: DuckedDry + Wet * Mix
                 float mixed = (filteredDry * currentDuckGain) + (finalWet * mixPct);

                 // Hard limiting at -0.01dB instead of soft clipping
                 const float hardLimitThreshold = juce::Decibels::decibelsToGain(-0.01f);
                 if (useSoftClip) {
                     if (mixed > hardLimitThreshold) mixed = hardLimitThreshold;
                     else if (mixed < -hardLimitThreshold) mixed = -hardLimitThreshold;
                 }

                 channelData[sample] = mixed;

                 // === Envelope Peak Aggregation (for EnvelopeView) ===
                 // Only process once per sample (ch == 0)
                 if (ch == 0)
                 {
                     // Accumulate peak values (linear amplitude)
                     float currentDetector = std::abs(detectorEnv);
                     float currentSynth = std::abs(envAmplitude);
                     float currentOutput = std::abs(mixed);

                     if (currentDetector > peakDetector) peakDetector = currentDetector;
                     if (currentSynth > peakSynthesizer) peakSynthesizer = currentSynth;
                     if (currentOutput > peakOutput) peakOutput = currentOutput;

                     envSampleCounter++;

                     // Every 128 samples, push aggregated peak to FIFO (unless frozen)
                     if (envSampleCounter >= envUpdateRate)
                     {
                         if (!isFrozen.load())
                         {
                             EnvelopeDataPoint dataPoint;
                             dataPoint.detector = peakDetector;
                             dataPoint.synthesizer = peakSynthesizer;
                             dataPoint.output = peakOutput;

                             // Write to FIFO (lock-free)
                             int start1, size1, start2, size2;
                             envelopeFifo.prepareToWrite(1, start1, size1, start2, size2);

                             if (size1 > 0)
                                 envelopeBuffer[start1] = dataPoint;

                             envelopeFifo.finishedWrite(size1);
                         }
                         else
                         {
                             // When frozen, clear FIFO to prevent accumulation
                             envelopeFifo.reset();
                         }

                         // Reset for next window
                         envSampleCounter = 0;
                         peakDetector = 0.0f;
                         peakSynthesizer = 0.0f;
                         peakOutput = 0.0f;
                     }
                 }
             }
        }
    }
    
    // AGM with +6dB max constraint and -60dB safety threshold
    float currentOutputRMS = buffer.getRMSLevel(0, 0, numSamples);
    outputRMS = currentOutputRMS;
    if (agmParam->load() > 0.5f) {
        float target = 1.0f;
        const float minThreshold = juce::Decibels::decibelsToGain(-60.0f);
        const float maxGain = juce::Decibels::decibelsToGain(6.0f);

        if (currentOutputRMS > minThreshold && currentInputRMS > minThreshold) {
            float computedGain = currentInputRMS / currentOutputRMS;
            target = juce::jlimit(0.1f, maxGain, computedGain);
        }
        agmGain.setTargetValue(target);
    } else { agmGain.setTargetValue(1.0f); }
    agmGain.applyGain(buffer, buffer.getNumSamples());

    // Soft Clipper: -0.01dB limiting with +0.01dB makeup gain
    const float limitThreshold = juce::Decibels::decibelsToGain(-0.01f);
    const float makeupGain = juce::Decibels::decibelsToGain(0.01f);

    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float value = channelData[sample];

            // Apply limiting
            if (value > limitThreshold)
                value = limitThreshold;
            else if (value < -limitThreshold)
                value = -limitThreshold;

            // Apply makeup gain
            channelData[sample] = value * makeupGain;
        }
    }
}

bool NewProjectAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor() { return new NewProjectAudioProcessorEditor (*this); }
void NewProjectAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // Detector
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("THRESHOLD", 1), "Thresh", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("CEILING", 1), "Ceiling", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), 0.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("DET_REL", 1), "Rel", juce::NormalisableRange<float>(1.0f, 500.0f, 1.0f), 20.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("WAIT_MS", 1), "Wait", juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f), 150.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("F_FREQ", 1), "F.Freq", juce::NormalisableRange<float>(20.0f, 10000.0f, 1.0f, 0.3f), 120.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("F_Q", 1), "F.Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("AUDITION", 1), "Audition", juce::StringArray("Off", "On"), 0));

    // Generator
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("START_FREQ", 1), "Start", juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f, 0.4f), 50.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("PEAK_FREQ", 1), "Peak", juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f, 0.4f), 80.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("SHAPE", 1), "Shape", juce::StringArray("Sine", "Triangle", "Square"), 0));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("SATURATION", 1), "Sat", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 25.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("NOISE_MIX", 1), "Noise", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 20.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    
    // Envelope
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("P_ATT", 1), "P.Att", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 10.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("P_DEC", 1), "P.Dec", juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f), 150.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("A_ATT", 1), "A.Att", juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 2.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("A_DEC", 1), "A.Dec", juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f), 400.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    
    // Output
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("DUCKING", 1), "Duck dB", juce::NormalisableRange<float>(-48.0f, 0.0f, 0.1f), -12.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("D_ATT", 1), "D.Att", juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 2.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("D_DEC", 1), "D.Dec", juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f), 300.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("WET_GAIN", 1), "Wet dB", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), -6.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("DRY_MIX", 1), "Dry %", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("MIX", 1), "Mix %", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f, "", juce::AudioProcessorParameter::genericParameter, nullptr, nullptr));
    layout.add (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("AGM_MODE", 1), "AGM", juce::StringArray("Off", "On"), 0));
    layout.add (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("SOFT_CLIP", 1), "Clip", juce::StringArray("Off", "On"), 1));
    layout.add (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("THEME", 1), "Theme", juce::StringArray("Bronze", "Blue", "Purple", "Green", "Pink"), 0));
    
    return layout;
}

void NewProjectAudioProcessor::setParameterValue(const juce::String& paramID, float value)
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts->getParameter(paramID)))
        param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(value));
    else if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(apvts->getParameter(paramID)))
        choice->setValueNotifyingHost(choice->getNormalisableRange().convertTo0to1(value));
}

void NewProjectAudioProcessor::loadPreset(int presetIndex)
{
    switch (presetIndex)
    {
        case 1:
            setParameterValue("START_FREQ", 60.0f); setParameterValue("PEAK_FREQ", 120.0f);
            setParameterValue("P_DEC", 20.0f); setParameterValue("NOISE_MIX", 0.0f); break;
        case 2:
            setParameterValue("START_FREQ", 200.0f); setParameterValue("PEAK_FREQ", 450.0f); break;
        case 3:
            setParameterValue("START_FREQ", 50.0f); setParameterValue("PEAK_FREQ", 800.0f); break;
        case 4:
            setParameterValue("START_FREQ", 40.0f); setParameterValue("PEAK_FREQ", 60.0f);
            setParameterValue("SHAPE", 2.0f); setParameterValue("SATURATION", 80.0f); break;
    }
}

void NewProjectAudioProcessor::updateFilterCoefficients(float freq, float Q) {
    if (currentSampleRate <= 0) return;
    float w0 = 2.0f * juce::MathConstants<float>::pi * freq / (float)currentSampleRate;
    float alpha = std::sin(w0) / (2.0f * Q);
    float b0 = alpha; float b1 = 0.0f; float b2 = -alpha;
    float a0 = 1.0f + alpha; float a1 = -2.0f * std::cos(w0); float a2 = 1.0f - alpha;
    bf0 = b0/a0; bf1 = b1/a0; bf2 = b2/a0; af1 = a1/a0; af2 = a2/a0;
}

void NewProjectAudioProcessor::updateEnvelopeIncrements(float pAtt, float pDec, float aAtt, float aDec, float dAtt, float dDec, float detRel) {
    float sr_ms = (float)currentSampleRate / 1000.0f;
    pitchAttackInc = 1.0f / (pAtt * sr_ms + 1.0f);
    pitchDecayInc = 1.0f / (pDec * sr_ms + 1.0f);
    ampAttackInc = 1.0f / (aAtt * sr_ms + 1.0f);
    ampDecayInc = 1.0f / (aDec * sr_ms + 1.0f);
    duckAttackInc = 1.0f / (dAtt * sr_ms + 1.0f);
    duckDecayInc = 1.0f / (dDec * sr_ms + 1.0f);
    detectorReleaseCoeff = 1.0f - std::exp(-1.0f / (detRel * sr_ms));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NewProjectAudioProcessor(); }
