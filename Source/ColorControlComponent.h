/*
  ==============================================================================
    ColorControlComponent.h
    Dynamic COLOR control with Amount knob + Attack/Decay envelope
    Replaces Saturation with multi-oscillator harmonic mixing
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Theme.h"

class ColorControlComponent : public juce::Component, private juce::Timer
{
public:
    ColorControlComponent(juce::AudioProcessorValueTreeState& apvts);
    ~ColorControlComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setPalette(const ThemePalette& palette);

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    juce::AudioProcessorValueTreeState& apvts;

    // COLOR Amount控制（主旋钮，0-100%）
    juce::Slider colorAmountSlider;
    std::unique_ptr<SliderAttachment> colorAmountAtt;

    // Attack/Decay时间控制（水平滑块）
    juce::Slider colorAttackSlider;
    juce::Slider colorDecaySlider;
    std::unique_ptr<SliderAttachment> colorAttackAtt;
    std::unique_ptr<SliderAttachment> colorDecayAtt;

    // 当前主题颜色
    ThemePalette currentPalette;

    // 包络可视化参数
    float currentEnvValue = 0.0f;  // 0.0-1.0，用于绘制包络动画

    // 绘制函数
    void drawEnvelopeVisualizer(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawKnob(juce::Graphics& g, juce::Rectangle<int> bounds, float value, const juce::String& label);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorControlComponent)
};
