/*
  ==============================================================================
    ColorControlComponent.cpp
    Dynamic COLOR control implementation
  ==============================================================================
*/

#include "ColorControlComponent.h"

ColorControlComponent::ColorControlComponent(juce::AudioProcessorValueTreeState& apvts)
    : apvts(apvts)
{
    // COLOR Amount旋钮（替换原Saturation）
    addAndMakeVisible(colorAmountSlider);
    colorAmountSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    colorAmountSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    colorAmountSlider.setRotaryParameters(juce::degreesToRadians(-145.0f), juce::degreesToRadians(145.0f), true);
    colorAmountAtt.reset(new SliderAttachment(apvts, "COLOR_AMOUNT", colorAmountSlider));

    // Attack滑块（水平）
    addAndMakeVisible(colorAttackSlider);
    colorAttackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    colorAttackSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    colorAttackAtt.reset(new SliderAttachment(apvts, "COLOR_ATT", colorAttackSlider));

    // Decay滑块（水平）
    addAndMakeVisible(colorDecaySlider);
    colorDecaySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    colorDecaySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    colorDecayAtt.reset(new SliderAttachment(apvts, "COLOR_DEC", colorDecaySlider));

    // 启动定时器用于动画（30fps）
    startTimerHz(30);
}

ColorControlComponent::~ColorControlComponent()
{
    stopTimer();
}

void ColorControlComponent::setPalette(const ThemePalette& palette)
{
    currentPalette = palette;

    // 应用主题颜色到控件
    auto applyColors = [&](juce::Slider& s) {
        s.setColour(juce::Slider::thumbColourId, palette.accent);
        s.setColour(juce::Slider::rotarySliderFillColourId, palette.accent);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, palette.panel900.brighter(0.2f));
        s.setColour(juce::Slider::trackColourId, palette.accent.withAlpha(0.6f));
        s.setColour(juce::Slider::backgroundColourId, palette.panel900.brighter(0.1f));
    };

    applyColors(colorAmountSlider);
    applyColors(colorAttackSlider);
    applyColors(colorDecaySlider);

    repaint();
}

void ColorControlComponent::timerCallback()
{
    // 获取当前包络值用于可视化（从处理器获取，暂时用模拟值）
    // TODO: 后续从processor获取实时包络状态
    repaint();
}

void ColorControlComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // 背景面板（工业风格，微妙白色叠加）
    g.setColour(juce::Colours::white.withAlpha(0.02f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // 边框（微妙高光）
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1.0f), 4.0f, 1.0f);

    // 旋钮区域
    auto knobArea = bounds.removeFromLeft(70).reduced(5);

    // COLOR标签（在旋钮下方）
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("COLOR", knobArea.getX(), knobArea.getBottom() - 12, knobArea.getWidth(), 12, juce::Justification::centred);

    // 数值显示（百分比）
    float amountValue = colorAmountSlider.getValue();
    juce::String valueText = juce::String(amountValue, 0) + "%";
    g.setColour(currentPalette.accent.withAlpha(0.9f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(valueText, knobArea.getX(), knobArea.getY() + 15, knobArea.getWidth(), 15, juce::Justification::centred);

    // 包络区域标签
    auto envArea = bounds.reduced(5);
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(juce::FontOptions(8.0f));

    // Attack标签
    g.drawText("ATT", envArea.getX(), envArea.getY() + 18, 30, 10, juce::Justification::left);
    float attValue = colorAttackSlider.getValue();
    g.drawText(juce::String(attValue, 1) + "ms", envArea.getX(), envArea.getY() + 28, 50, 10, juce::Justification::left);

    // Decay标签
    g.drawText("DEC", envArea.getX(), envArea.getY() + 48, 30, 10, juce::Justification::left);
    float decValue = colorDecaySlider.getValue();
    g.drawText(juce::String(decValue, 1) + "ms", envArea.getX(), envArea.getY() + 58, 50, 10, juce::Justification::left);

    // 绘制包络可视化（右侧小区域）
    auto vizArea = juce::Rectangle<int>(envArea.getRight() - 35, envArea.getY() + 20, 30, 50);
    drawEnvelopeVisualizer(g, vizArea);
}

void ColorControlComponent::drawEnvelopeVisualizer(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // 包络形状可视化（简化的Attack/Decay曲线）
    juce::Path envelopePath;

    float attValue = colorAttackSlider.getValue();
    float decValue = colorDecaySlider.getValue();
    float attMax = colorAttackSlider.getMaximum();
    float decMax = colorDecaySlider.getMaximum();

    // 安全检查
    if (attMax <= 0.0f) attMax = 50.0f;
    if (decMax <= 0.0f) decMax = 500.0f;

    float attRatio = attValue / attMax;
    float decRatio = decValue / decMax;

    float x = bounds.getX();
    float y = bounds.getBottom();
    float w = bounds.getWidth();
    float h = bounds.getHeight();

    // 起点
    envelopePath.startNewSubPath(x, y);

    // Attack上升
    float attackX = x + w * 0.3f * juce::jlimit(0.0f, 1.0f, attRatio);
    envelopePath.lineTo(attackX, y - h);

    // Decay下降
    float decayX = attackX + w * 0.7f * juce::jlimit(0.0f, 1.0f, decRatio);
    envelopePath.lineTo(decayX, y);

    // 尾部
    envelopePath.lineTo(x + w, y);

    // 绘制包络曲线
    g.setColour(currentPalette.accent.withAlpha(0.4f));
    g.strokePath(envelopePath, juce::PathStrokeType(1.5f));

    // 填充
    juce::Path fillPath = envelopePath;
    fillPath.lineTo(x + w, y);
    fillPath.lineTo(x, y);
    fillPath.closeSubPath();
    g.setColour(currentPalette.accent.withAlpha(0.1f));
    g.fillPath(fillPath);

    // 边框
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(bounds, 1);
}

void ColorControlComponent::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    // 左侧：Amount旋钮
    auto knobArea = bounds.removeFromLeft(60);
    colorAmountSlider.setBounds(knobArea.removeFromTop(60).reduced(5));

    // 右侧：Attack/Decay滑块
    auto sliderArea = bounds.reduced(5, 0);

    // Attack滑块（上半部分）
    auto attackArea = sliderArea.removeFromTop(25);
    colorAttackSlider.setBounds(attackArea.removeFromBottom(12).reduced(50, 0).translated(0, -5));

    sliderArea.removeFromTop(5);  // 间隔

    // Decay滑块（下半部分）
    auto decayArea = sliderArea.removeFromTop(25);
    colorDecaySlider.setBounds(decayArea.removeFromBottom(12).reduced(50, 0).translated(0, -5));
}
