/*
  ==============================================================================
    PluginEditor.h (SPLENTA V18.6 - 20251215.03)
    Theme System: External Change Synchronization
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeView.h"
#include "ThemeSelector.h"
#include "Theme.h"

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

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

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

    juce::TextButton expandButton { "" };
    bool showFFT = true;

    EnvelopeView envelopeView;
    float splitRatio = 0.5f;
    juce::Rectangle<int> envelopeArea, fftArea, dividerArea;

    // Splitter dragging
    enum DragAction { None, Splitter };
    DragAction currentDragAction = None;

    void setupKnob(juce::Slider& slider, const juce::String& id, std::unique_ptr<SliderAttachment>& attachment, const juce::String& suffix);
    void drawPixelArt(juce::Graphics& g, int x, int y, int scale, juce::Colour c, int type);
    void drawPixelHeadphone(juce::Graphics& g, int x, int y, int scale, juce::Colour c);

    // Theme change detection
    int lastThemeIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};
