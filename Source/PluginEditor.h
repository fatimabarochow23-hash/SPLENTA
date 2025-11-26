/*
  ==============================================================================
    PluginEditor.h (V18.3 - FIXED HEADERS)
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// --- 内部类：滚动示波器 ---
class RollingScope : public juce::Component, public juce::Timer
{
public:
    RollingScope(NewProjectAudioProcessor& p) : processor(p) { startTimerHz(60); }
    ~RollingScope() override { stopTimer(); }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::transparentBlack);

        float w = (float)getWidth();
        float h = (float)getHeight();
        float midY = h * 0.5f;
        float maxAmpHeight = midY - 15.0f;

        // 1. Draw Min/Max Waveform (Professional Style)
        g.setColour(lineColour);

        int peakWriteIndex = processor.peakWritePos.load();
        int totalPeaks = processor.peakBufferSize;

        // Draw each vertical line for each pixel X
        for (int x = 0; x < (int)w; ++x) {
            // Map screen X to peak buffer index
            int peakIndex = (peakWriteIndex - (int)w + x);
            while (peakIndex < 0) peakIndex += totalPeaks;
            peakIndex = peakIndex % totalPeaks;

            auto& peak = processor.peakBuffer[peakIndex];

            // Convert min/max to Y coordinates
            float minY = midY - (peak.minValue * maxAmpHeight);
            float maxY = midY - (peak.maxValue * maxAmpHeight);

            // Clamp to visible area
            minY = juce::jlimit(0.0f, h, minY);
            maxY = juce::jlimit(0.0f, h, maxY);

            // Draw vertical line from min to max (solid waveform effect)
            if (std::abs(maxY - minY) < 1.0f) {
                // For very small ranges, draw at least a 1px line for visibility
                g.drawVerticalLine(x, midY - 0.5f, midY + 0.5f);
            } else {
                g.drawVerticalLine(x, maxY, minY);
            }
        }

        // 2. 阈值线 (Threshold) & 梯形手柄
        float thVal = juce::Decibels::decibelsToGain(threshDB);
        threshY = midY - (thVal * maxAmpHeight);
        
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawHorizontalLine((int)threshY, 0.0f, w);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawHorizontalLine((int)(midY + (midY - threshY)), 0.0f, w); // Mirror

        // 梯形
        juce::Path thTrap;
        thTrap.startNewSubPath(0, threshY);
        thTrap.lineTo(25, threshY); thTrap.lineTo(20, threshY + 12); thTrap.lineTo(0, threshY + 12);
        thTrap.closeSubPath();
        g.setColour(juce::Colours::white.withAlpha(0.8f)); g.fillPath(thTrap);
        g.setColour(juce::Colours::black); g.setFont(10.0f);
        g.drawText("T", 2, (int)threshY, 20, 12, juce::Justification::centred);
        
        // 3. 上限线 (Ceiling) & 梯形手柄
        float ceilVal = juce::Decibels::decibelsToGain(ceilingDB);
        ceilingY = midY - (ceilVal * maxAmpHeight);
        
        g.setColour(juce::Colours::cyan.withAlpha(0.8f));
        g.drawHorizontalLine((int)ceilingY, 0.0f, w);
        
        juce::Path ceTrap;
        ceTrap.startNewSubPath(0, ceilingY);
        ceTrap.lineTo(25, ceilingY); ceTrap.lineTo(20, ceilingY - 12); ceTrap.lineTo(0, ceilingY - 12);
        ceTrap.closeSubPath();
        g.setColour(juce::Colours::cyan.withAlpha(0.8f)); g.fillPath(ceTrap);
        g.setColour(juce::Colours::black);
        g.drawText("C", 2, (int)ceilingY - 12, 20, 12, juce::Justification::centred);
    }
    
    void timerCallback() override { repaint(); }
    void setColors(juce::Colour line) { lineColour = line; }
    void setThresholds(float t, float c) { threshDB = t; ceilingDB = c; }
    
    float getThresholdY() const { return threshY; }
    float getCeilingY() const { return ceilingY; }
    float getScaleFactor() const { return getHeight() * 0.5f - 15.0f; }
    float getMidY() const { return getHeight() * 0.5f; }

private:
    NewProjectAudioProcessor& processor;
    juce::Colour lineColour = juce::Colours::orange;
    float threshDB = -20.0f; float ceilingDB = 0.0f;
    float threshY = 0.0f; float ceilingY = 0.0f;
};

// --- Editor ---
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

    juce::ComboBox shapeBox, themeBox, presetBox;
    std::unique_ptr<ComboBoxAttachment> shapeAtt, themeAtt;
    
    juce::TextButton agmButton { "Auto Gain" };
    std::unique_ptr<ButtonAttachment> agmAtt;
    juce::TextButton clipButton { "Soft Clip" };
    std::unique_ptr<ButtonAttachment> clipAtt;
    
    juce::TextButton auditionButton { "" };
    std::unique_ptr<ButtonAttachment> auditionAtt;

    juce::TextButton expandButton { "" };
    bool showFFT = true;

    RollingScope scopeComponent;
    float splitRatio = 0.5f;
    juce::Rectangle<int> scopeArea, fftArea, dividerArea;
    
    // [核心修复] 确保这些变量在 .h 中声明了！
    enum DragAction { None, Splitter, DragThreshold, DragCeiling };
    DragAction currentDragAction = None;
    bool isDraggingSplitter = false;
    bool isDraggingThresh = false;

    void setupKnob(juce::Slider& slider, const juce::String& id, std::unique_ptr<SliderAttachment>& attachment, const juce::String& suffix);
    void drawPixelArt(juce::Graphics& g, int x, int y, int scale, juce::Colour c, int type);
    void drawPixelHeadphone(juce::Graphics& g, int x, int y, int scale, juce::Colour c);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};
