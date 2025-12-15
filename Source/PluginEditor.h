/*
  ==============================================================================
    PluginEditor.h (SPLENTA V18.6 - 20251216.05)
    Batch 03: UI Reorganization + Value Show-on-Interaction
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeView.h"
#include "ThemeSelector.h"
#include "Theme.h"
#include "StealthLookAndFeel.h"

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

    juce::ComboBox shapeBox, presetBox;
    std::unique_ptr<ComboBoxAttachment> shapeAtt;

    // Theme selector (replaces old themeBox)
    ThemeSelector themeSelector;

    juce::TextButton agmButton { "Auto Gain" };
    std::unique_ptr<ButtonAttachment> agmAtt;
    juce::TextButton clipButton { "Soft Clip" };
    std::unique_ptr<ButtonAttachment> clipAtt;

    juce::TextButton auditionButton { "" };
    std::unique_ptr<ButtonAttachment> auditionAtt;

    EnvelopeView envelopeView;
    juce::Rectangle<int> envelopeArea, topologyArea;

    void setupKnob(juce::Slider& slider, const juce::String& id, std::unique_ptr<SliderAttachment>& attachment, const juce::String& suffix);
    void drawPixelArt(juce::Graphics& g, int x, int y, int scale, juce::Colour c, int type);
    void drawPixelHeadphone(juce::Graphics& g, int x, int y, int scale, juce::Colour c);

    // Theme change detection
    int lastThemeIndex = -1;

    // Custom LookAndFeel for interactive feedback
    StealthLookAndFeel stealthLnF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};
