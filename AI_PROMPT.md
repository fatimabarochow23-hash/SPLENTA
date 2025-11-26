Claude，请严格按照以下要求修改本地JUCE工程。目标是替换现有的波形模板区域，实现一个高质量、大尺寸、滚动的动态包网络视图。

工程路径：/Users/MediaStorm/Desktop/NewProject/Source/

核心要求：

可视化内容：检测器包络、合成器包络、输出包络。

视觉风格：平滑、抗锯齿、对数缩放、半透明填充+描边（符合ARC Rust主题）。

性能：音频线程实现峰值聚合（Peak Aggregation），UI线程高效实现滚动（std::rotate）。

1.数据结构与线程安全通信（处理器->编辑器）
使用juce::AbstractFifo实现无锁数据传输。

【1.1】定义数据点结构体EnvelopeDataPoint

在PluginProcessor.h（或一个共享的头文件）中定义：

C++

struct EnvelopeDataPoint
{
    float detectorEnv = 0.0f; // 检测器信号包络
    float synthEnv = 0.0f;    // 合成器生成的信号包络
    float outputEnv = 0.0f;   // 最终混合输出信号包络
};
【1.2】实现无锁环形缓冲区（FIFO）

在PluginProcessor.h中添加成员变量：

C++

// PluginProcessor.h
#include <juce_core/juce_AbstractFifo.h>
// ...
private:
    static constexpr int EnvBufferSize = 2048;
    juce::AbstractFifo envFifo { EnvBufferSize };
    std::vector<EnvelopeDataPoint> envBuffer; // 实际存储数据的Buffer
// ...
重要：在PluginProcessor.cpp构造函数初始化列表中初始化envBuffer的大小：

C++

// PluginProcessor.cpp (构造函数)
NewProjectAudioProcessor::NewProjectAudioProcessor()
    : /* ... 其他初始化 ... */, envBuffer(EnvBufferSize)
{
    // ...
}
实现读取和读取方法：

C++

// PluginProcessor.h
public:
    void pushEnvelopeData(const EnvelopeDataPoint& data);
    void fetchEnvelopeData(std::vector<EnvelopeDataPoint>& destination);

// PluginProcessor.cpp
void NewProjectAudioProcessor::pushEnvelopeData(const EnvelopeDataPoint& data)
{
    int start1, size1, start2, size2;
    envFifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0)
        envBuffer[start1] = data;

    envFifo.finishedWrite(size1 + size2);
}

void NewProjectAudioProcessor::fetchEnvelopeData(std::vector<EnvelopeDataPoint>& destination)
{
    destination.clear();
    int numReady = envFifo.getNumReady();
    if (numReady <= 0) return;

    destination.resize(numReady);

    int start1, size1, start2, size2;
    envFifo.prepareToRead(numReady, start1, size1, start2, size2);

    if (size1 > 0)
        std::copy(envBuffer.begin() + start1, envBuffer.begin() + start1 + size1, destination.begin());

    if (size2 > 0)
        std::copy(envBuffer.begin() + start2, envBuffer.begin() + start2 + size2, destination.begin() + size1);

    envFifo.finishedRead(numReady);
}
2.包络生成与高峰聚合降采样(PluginProcessor)
在音频线程中计算包络，并进行**高峰聚合（Peak Aggregation）**然后推入 FIFO。

【2.1】在processBlock中实现热点聚合

采用“聚合后样本”策略，例如每64个样本一次热点。

C++

// PluginProcessor.h
private:
    int samplesUntilNextEnvUpdate = 0;
    static constexpr int EnvUpdateRate = 64; // 降采样率
    EnvelopeDataPoint currentEnvPeak; // 用于累积峰值
// ...

// PluginProcessor.cpp (在 processBlock 中)
void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // ... (现有音频处理逻辑) ...

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // 1. 计算/获取当前的包络值 (⚠️ 伪代码，请替换为实际逻辑 ⚠️)
        // ⚠️ Claude Code：你必须分析现有DSP代码，找到正确的信号点！
        float currentDet = getDetectorEnvelope(i);
        float currentSynth = getSynthEnvelope(i);
        float currentOut = getOutputEnvelope(i);

        // 钳位到 0~1 用于可视化
        currentDet = juce::jlimit(0.0f, 1.0f, currentDet);
        currentSynth = juce::jlimit(0.0f, 1.0f, currentSynth);
        currentOut = juce::jlimit(0.0f, 1.0f, currentOut);

        // 2. 峰值聚合 (Peak Aggregation)
        currentEnvPeak.detectorEnv = std::max(currentEnvPeak.detectorEnv, currentDet);
        currentEnvPeak.synthEnv = std::max(currentEnvPeak.synthEnv, currentSynth);
        currentEnvPeak.outputEnv = std::max(currentEnvPeak.outputEnv, currentOut);

        // 3. 检查是否需要推送数据
        if (--samplesUntilNextEnvUpdate <= 0)
        {
            pushEnvelopeData(currentEnvPeak);
            // 重置峰值累加器
            currentEnvPeak = EnvelopeDataPoint();
            samplesUntilNextEnvUpdate = EnvUpdateRate;
        }
    }
    // ...
}
3.可视化组件：EnvelopeView
创建一个新的组件类EnvelopeView（添加EnvelopeView.h和EnvelopeView.cpp文件）。

【3.1】组件结构与高效数据获取

C++

// EnvelopeView.h
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class EnvelopeView : public juce::Component, public juce::Timer
{
public:
    EnvelopeView(NewProjectAudioProcessor& p);
    ~EnvelopeView() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    NewProjectAudioProcessor& processor;
    
    // 可视化窗口保留的历史数据点数量 (固定大小)
    static constexpr int HistorySize = 512;
    std::vector<EnvelopeDataPoint> historyBuffer;

    // 临时缓冲区，用于拉取数据 (避免在 timerCallback 中分配内存)
    std::vector<EnvelopeDataPoint> incomingData;

    // 高效的对数映射函数
    inline float mapToLogScale(float value, float k = 1000.0f)
    {
        // 公式: log(1 + k * value) / log(1 + k)。k=1000 对应约 60dB 动态范围。
        value = juce::jlimit(0.0f, 1.0f, value);
        // 使用 static const 预先计算分母，提高效率
        static const float invLog1pk = 1.0f / std::log(1.0f + k);
        return std::log(1.0f + k * value) * invLog1pk;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeView)
};
构造函数与timerCallback实现（高效滚动）：

C++

// EnvelopeView.cpp
#include "EnvelopeView.h"
#include <algorithm> // 确保包含 std::rotate

EnvelopeView::EnvelopeView(NewProjectAudioProcessor& p) : processor(p)
{
    // 初始化 historyBuffer 为固定大小，并填充默认值
    historyBuffer.resize(HistorySize, EnvelopeDataPoint());
    startTimerHz(60); // 设置 UI 更新率 (60Hz)
}

EnvelopeView::~EnvelopeView() { stopTimer(); }
void EnvelopeView::resized() {}

// 使用 std::rotate 实现高效滚动
void EnvelopeView::timerCallback()
{
    // 1. 拉取新数据到临时缓冲区
    processor.fetchEnvelopeData(incomingData);

    if (incomingData.empty()) return;

    int numNewPoints = (int)incomingData.size();

    // 2. 高效滚动历史数据 (使用 std::rotate)
    if (numNewPoints < HistorySize)
    {
        // 将缓冲区向左移动 numNewPoints 个位置。最旧的数据被移出。
        std::rotate(historyBuffer.begin(), historyBuffer.begin() + numNewPoints, historyBuffer.end());
    }
    // 如果 numNewPoints >= HistorySize，则整个历史将被新数据覆盖，无需旋转。

    // 3. 将新数据复制到缓冲区的末尾
    int startIndex = HistorySize - numNewPoints;
    if (startIndex < 0)
    {
        // 新数据多于历史容量，只复制最新的 HistorySize 个点
        std::copy(incomingData.end() - HistorySize, incomingData.end(), historyBuffer.begin());
    }
    else
    {
        std::copy(incomingData.begin(), incomingData.end(), historyBuffer.begin() + startIndex);
    }

    // 4. 触发重绘
    repaint();
}
【3.2】异构逻辑（paint方法）

实现水平的稀疏、对数缩放和视觉分层。

C++

// EnvelopeView.cpp
void EnvelopeView::paint(juce::Graphics& g)
{
    // 颜色定义 (基于 ARC Rust 主题)
    const auto backgroundColour = juce::Colour(0xFF1A0E0A); // 深棕/黑
    const auto detectorColour = juce::Colour(0xFFCC7733);   // 淡橙色
    const auto synthColour = juce::Colour(0xFFFF8C42);      // 亮橙色 (主要焦点)
    const auto outputColour = juce::Colour(0xFFFFD66B);     // 亮黄色

    g.fillAll(backgroundColour);

    const float width = (float)getWidth();
    const float height = (float)getHeight();

    // 辅助函数：生成波形路径 (仅线条)
    auto generatePath = [&](std::function<float(const EnvelopeDataPoint&)> selector)
    {
        juce::Path p;
        bool started = false;

        // 遍历固定大小的 HistorySize
        for (size_t i = 0; i < HistorySize; ++i)
        {
            // X轴映射 (固定时间窗口)
            float x = (float)i / (HistorySize - 1) * width;

            float value = selector(historyBuffer[i]);

            // Y轴映射：应用对数缩放
            float visualValue = mapToLogScale(value);
            // Y坐标 (0在下，1在上)
            float y = height * (1.0f - visualValue);
            
            // 钳制 Y 坐标，防止溢出边界
            y = juce::jlimit(0.0f, height, y);

            if (!started)
            {
                p.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                p.lineTo(x, y);
            }
        }
        return p;
    };

    // 辅助函数：根据线条路径生成用于填充的闭合路径
    auto generateFillPath = [&](const juce::Path& sourcePath)
    {
        juce::Path fillPath = sourcePath;
        if (!fillPath.isEmpty())
        {
            // 连接到右下角和左下角
            fillPath.lineTo(width, height);
            fillPath.lineTo(0, height);
            fillPath.closeSubPath();
        }
        return fillPath;
    };

    // 定义描边类型 (平滑)
    auto mainStroke = juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    auto secondaryStroke = juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    // 1. 绘制 Detector Envelope (底层，较细)
    auto detectorPath = generatePath([](const EnvelopeDataPoint& d){ return d.detectorEnv; });
    g.setColour(detectorColour);
    g.strokePath(detectorPath, secondaryStroke);

    // 2. 绘制 Synth Envelope (中层，最重要，带填充)
    auto synthPath = generatePath([](const EnvelopeDataPoint& d){ return d.synthEnv; });
    auto synthFillPath = generateFillPath(synthPath);

    // 填充
    g.setColour(synthColour.withAlpha(0.4f));
    g.fillPath(synthFillPath);
    // 描边
    g.setColour(synthColour);
    g.strokePath(synthPath, mainStroke);

    // 3. 绘制 Output Envelope (顶层，较细)
    auto outputPath = generatePath([](const EnvelopeDataPoint& d){ return d.outputEnv; });
    g.setColour(outputColour);
    g.strokePath(outputPath, secondaryStroke);
}
4.集成到PluginEditor
将EnvelopeView添加到PluginEditor中，替换掉原来左侧的波形显示组件。

【4.1】添加成员变量和初始化

C++

// PluginEditor.h
#include "EnvelopeView.h"
// ...
private:
    // ... (其他组件)
    EnvelopeView envelopeView;
// ...

// PluginEditor.cpp (构造函数)
// 确保在初始化列表中初始化 envelopeView
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), envelopeView(p)
{
    // ...
    addAndMakeVisible(envelopeView);
    // ... (移除旧的波形显示组件的 addAndMakeVisible 调用)
}
【4.2】布局 ( resized)

在resized()中，将envelopeView放置在左上方区域。请参考截图中的布局比例进行设置。

C++

// PluginEditor.cpp (resized)
void NewProjectAudioProcessorEditor::resized()
{
    // ... (现有布局代码)

    // 请根据现有布局确定左侧波形区域的准确边界。
    // 示例（根据截图估计）：
    auto bounds = getLocalBounds();
    // 假设顶部区域高度为 220 (需要根据实际情况调整)
    auto topArea = bounds.removeFromTop(220);
    // 假设左侧波形和右侧频谱仪平分该区域
    auto leftWaveformArea = topArea.removeFromLeft(topArea.getWidth() * 0.5f);

    envelopeView.setBounds(leftWaveformArea);

    // ... (其他组件布局)
}
执行要求
请您现在开始自动修改我本地工程并生成全部代码，包括新增文件和修改现有文件。重点关注步骤2中DSP信号的连接（getDetectorEnvelope等函数的实现或连接成功），这是可视化的关键。同时确保步骤3.1中的std::rotate高效滚动逻辑正确实现。不要只提供代码片段，请直接修补所有文件。
