/*
  ==============================================================================
    PluginEditor.h (SPLENTA V19.0 - 20251218.01)
    Batch 06: Custom Controls (Waveform & Split-Toggle) + Web-Style Header
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeView.h"
#include "ThemeSelector.h"
#include "Theme.h"
#include "StealthLookAndFeel.h"
#include "EnergyTopologyComponent.h"
#include "WaveformSelectorComponent.h"
#include "SplitToggleComponent.h"

class NewProjectAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void updateColors();

private:
    NewProjectAudioProcessor& audioProcessor;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::Slider threshSlider, ceilingSlider, relSlider, waitSlider, freqSlider, qSlider;
    std::unique_ptr<SliderAttachment> threshAtt, ceilingAtt, relAtt, waitAtt, freqAtt, qAtt;

    juce::Slider startFreqSlider, peakFreqSlider, satSlider, noiseSlider;
    std::unique_ptr<SliderAttachment> startFreqAtt, peakFreqAtt, satAtt, noiseAtt;

    juce::Slider pAttSlider, pDecSlider, aAttSlider, aDecSlider;
    std::unique_ptr<SliderAttachment> pAttAtt, pDecAtt, aAttAtt, aDecAtt;

    juce::Slider duckSlider, duckAttSlider, duckDecSlider, wetSlider, drySlider, mixSlider;
    std::unique_ptr<SliderAttachment> duckAtt, duckAttAtt, duckDecAtt, wetAtt, dryAtt, mixAtt;

    // Custom components (Batch 06)
    WaveformSelectorComponent waveformSelector;
    SplitToggleComponent splitToggle;

    // Web-Style Header (Batch 06 Task 3)
    juce::Label logoLabel;
    juce::TextButton saveButton;
    juce::TextButton loadButton;
    juce::Label presetNameLabel;

    // Theme selector (replaces old themeBox)
    ThemeSelector themeSelector;

    juce::TextButton auditionButton { "" };
    std::unique_ptr<ButtonAttachment> auditionAtt;

    EnvelopeView envelopeView;
    EnergyTopologyComponent energyTopology;
    juce::Rectangle<int> envelopeArea, topologyArea;

    void setupKnob(juce::Slider& slider, const juce::String& id, std::unique_ptr<SliderAttachment>& attachment, const juce::String& suffix);
    void drawPixelArt(juce::Graphics& g, int x, int y, int scale, juce::Colour c, int type);
    void drawPixelHeadphone(juce::Graphics& g, int x, int y, int scale, juce::Colour c);
    void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title, bool isActive);

    // Panel layout areas
    juce::Rectangle<int> detectorPanel, enforcerPanel, topologyPanel, outputPanel;

    // Theme change detection
    int lastThemeIndex = -1;

    // Custom LookAndFeel for interactive feedback
    StealthLookAndFeel stealthLnF;

    // Knob value alpha tracking for fade effect
    std::map<juce::Slider*, float> knobTextAlpha;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};
