🚀 任务：V18.6 全面升级 - 高级可视化、性能与 DSP 优化Claude，本次迭代(V18.6)包含大量关键性修改和优化。请严格按照以下自定义指令执行。工程路径：/Users/MediaStorm/Desktop/NewProject/Source/￼一模块：时间窗口重构与性能优化解决滚动速度过快问题，将默认时间窗口漂移约6秒。同时实现精确的滚动归零逻辑。【1.1】参数调整与采样率同步1. PluginProcessor.h修改:• EnvBufferSize：到增加4096。• EnvUpdateRate：从64增加到128(平衡性能和时间跨度)。• 添加原子指标存储采样率：std::atomic<double> currentSampleRate { 44100.0 };• 公共添加方法double getSampleRate() const { return currentSampleRate.load(); }。2. PluginProcessor.cpp修改:• 在prepareToPlay中更新采样率：currentSampleRate.store(sampleRate);。• 保证构造函数中envBuffer初始化大小正确。3. EnvelopeView.h修改:• HistorySize：从512增加到2048。• 添加元件指标juce::int64 lastCallbackTime = 0;用于精确计时。【1.2】实现冻结功能与精确滚动归零（Scroll Away）实现当声音停止时，波形自然滚动消失的逻辑。1. PluginEditor：• 添加一个juce::ToggleButton，命名为freezeButton，文本“Freeze”。放置在可视化区域的右上角。• 点击按钮时更新状态，并调用envelopeView.setFrozen(...)（重要：也必须调用 FFT 视图的响应调用方法）。2. EnvelopeView.h：• 添加std::atomic<bool> isFrozen { false };和setFrozen(bool)方法。3. EnvelopeView.cpp::timerCallback（核心逻辑重构）：C++￼void EnvelopeView::timerCallback()
{
    if (isFrozen.load())
    {
        // 如果冻结，清空 FIFO 数据防止堆积，然后返回
        processor.fetchEnvelopeData(incomingData);
        incomingData.clear();
        lastCallbackTime = 0; // 重置计时器
        return;
    }

    // 1. 精确计时 (使用高精度时钟)
    juce::int64 currentTime = juce::Time::currentTimeMillis();
    // 计算自上次回调以来的经过时间 (秒)
    double elapsedSec = (lastCallbackTime == 0) ? 0.0 : (currentTime - lastCallbackTime) / 1000.0;
    lastCallbackTime = currentTime;

    // 2. 拉取新数据
    processor.fetchEnvelopeData(incomingData);
    int numNewPoints = (int)incomingData.size();

    // 3. ‼️ 精确滚动归零逻辑 (当音频停止时) ‼️
    if (numNewPoints == 0)
    {
        if (elapsedSec <= 0) return;

        // 计算这段时间内应该产生多少个数据点
        double sampleRate = processor.getSampleRate();
        // ⚠️ 必须使用 PluginProcessor 中的 EnvUpdateRate 常量 (128)
        const int updateRate = NewProjectAudioProcessor::EnvUpdateRate;
        
        if (sampleRate > 0 && updateRate > 0)
        {
            double pointsPerSec = sampleRate / (double)updateRate;
            int pointsToInject = (int)std::round(pointsPerSec * elapsedSec);

            if (pointsToInject > 0)
            {
                // 注入静音数据以模拟时间流逝，使波形滚动消失
                numNewPoints = std::min(pointsToInject, HistorySize);
                incomingData.resize(numNewPoints, EnvelopeDataPoint()); // 填充零值
            }
        }

        if (numNewPoints == 0) return;
    }

    // 4. 高效滚动 (std::rotate)
    if (numNewPoints < HistorySize)
    {
        std::rotate(historyBuffer.begin(), historyBuffer.begin() + numNewPoints, historyBuffer.end());
    }
    
    // 5. 复制新数据
    int startIndex = HistorySize - numNewPoints;
    if (startIndex < 0)
    {
         std::copy(incomingData.end() - HistorySize, incomingData.end(), historyBuffer.begin());
    }
    else
    {
        std::copy(incomingData.begin(), incomingData.end(), historyBuffer.begin() + startIndex);
    }

    repaint();
}
⚠️重要：请对FFT视图也应用类似的基于精确计时的归零逻辑。￼模块二：UI/UX增强与主题动力【2.1】颜色主题自适应（EnvelopeView.cpp）修改paint方法，停止使用硬编码颜色，改为从LookAndFeel动态获取标准颜色ID。C++￼// EnvelopeView.cpp::paint()

// 获取主题颜色
const auto backgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);

// 使用旋钮填充色作为主强调色 (例如 ARC Rust 的橙色，Pro Purple 的紫色)
const auto accentColour = getLookAndFeel().findColour(juce::Slider::rotarySliderFillColourId);
// 使用 Thumb 色作为参考线颜色 (T/C 线)
const auto referenceColour = getLookAndFeel().findColour(juce::Slider::thumbColourId); 

// 定义波形颜色
const auto synthColour = accentColour;
// 使用变体以区分 Detector 和 Output
const auto detectorColour = accentColour.brighter(0.5f).desaturated(0.3f);
const auto outputColour = accentColour.darker(0.5f).brighter(0.1f); // 确保输出可见性

// ... (在后续绘制调用中使用这些变量) ...
【2.2】阈值/天花板（T/C）线条与标签美化修改EnvelopeView::paint中的T/C 逻辑较弱。1. 线条样式：Threshold 和 Ceiling 改为实线（去除虚线逻辑）。Ceiling 1.8f，Threshold 1.2f（轻微透明）。2. 标签重做（梯形样式）：实现场景局部函数drawIndicator。C++￼// EnvelopeView.cpp (添加辅助函数)
void drawIndicator(juce::Graphics& g, float yPos, const juce::String& text, const juce::Colour& colour, const juce::Colour& textColour)
{
    const float width = 22.0f;
    const float height = 18.0f;
    const float xPos = 2.0f; // 边距

    // 钳位 Y 坐标，防止标签超出视图边界
    yPos = juce::jlimit(height * 0.5f, (float)getHeight() - height * 0.5f, yPos);

    // 绘制梯形背景 (使用 Path)
    juce::Path p;
    p.startNewSubPath(xPos, yPos - height * 0.5f);
    p.lineTo(xPos + width * 0.7f, yPos - height * 0.5f);
    p.lineTo(xPos + width, yPos); // 右侧尖角
    p.lineTo(xPos + width * 0.7f, yPos + height * 0.5f);
    p.lineTo(xPos, yPos + height * 0.5f);
    p.closeSubPath();

    g.setColour(colour);
    g.fillPath(p);

    // 绘制文本
    g.setColour(textColour); // 使用背景色形成对比
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    juce::Rectangle<float> textArea(xPos, yPos - height * 0.5f, width * 0.6f, height);
    g.drawText(text, textArea, juce::Justification::centred, false);
}

// 在 paint 方法中调用 (确保在绘制线条后调用)
// ... 绘制线条逻辑 ...
drawIndicator(g, ceilingY, "C", referenceColour, backgroundColour);
drawIndicator(g, thresholdY, "T", referenceColour.withAlpha(0.8f), backgroundColour);
【2.3】FFT折叠按钮优化(PluginEditor.h/cpp)• 任务：找到PluginEditor中控制FFT折叠的按钮。将其替换为更大的圆形按钮（直径约24px）。修改其相似逻辑（可能需要自定义LookAndFeel或重写paintButton），实现圆形和背景声音的箭头图标。确保箭头方向随折叠状态变化，且点击区域足够大。￼模块三：动态时间缩放与“狙击镜”效果（核心）实现当FFT折叠时，EnvelopeView的时间窗口动态缩放到释放时间，并对应动态形状的狙击镜()。【3.1】 状态联动1. EnvelopeView.h: 添加std::atomic<bool> isDynamicZoomActive { false };和setDynamicZoomActive(bool)方法。2. PluginEditor.cpp：在FFT折叠状态改变时（按钮点击处理和resized中），判断FFT折叠是否，并调用envelopeView.setDynamicZoomActive(...)。【3.2】动态缩放实现（EnvelopeView.cpp::paint）重构paint方法以支持动态缩放。C++￼// EnvelopeView.cpp::paint()

// ... (初始化代码，获取 width, height) ...

double sampleRate = processor.getSampleRate();
if (sampleRate <= 0) sampleRate = 44100.0; // 容错

// 1. 确定要绘制的点数 (numPointsToDraw)
int numPointsToDraw = HistorySize; // 默认为固定窗口

// ⚠️ 请替换为正确的 Detector Release 参数 ID (例如 "DET_REL")
float releaseMs = processor.getAPVTS().getRawParameterValue("DET_REL")->load(); 

if (isDynamicZoomActive.load())
{
    // 动态缩放模式激活
    const int updateRate = NewProjectAudioProcessor::EnvUpdateRate;
    double pointsPerSecond = sampleRate / (double)updateRate;
    
    // 将 Release 时间映射到点数 (我们让视图稍微比 Release 时间长一点，提供上下文)
    float targetTimeMs = releaseMs * 1.2f; 

    int dynamicPoints = (int)((targetTimeMs / 1000.0) * pointsPerSecond);
    
    // 限制范围 (最小 128 点，最大 HistorySize)
    numPointsToDraw = juce::jlimit(128, HistorySize, dynamicPoints);
}

// 2. 修改 generatePath 以适应动态缩放 (关键)
auto generatePath = [&](std::function<float(const EnvelopeDataPoint&)> selector)
{
    juce::Path p;
    bool started = false;

    // 只遍历 numPointsToDraw 个点
    for (int i = 0; i < numPointsToDraw; ++i)
    {
        // X轴映射：将 numPointsToDraw 个点映射到整个 width
        float x = (float)i / (numPointsToDraw - 1) * width;

        // 从 historyBuffer 的末尾 (最新数据) 向前读取
        int bufferIndex = HistorySize - numPointsToDraw + i;
        
        // 确保索引安全
        if (bufferIndex < 0 || bufferIndex >= HistorySize) continue;

        // ... (获取 value, 计算 y 坐标，保持不变) ...
        float value = selector(historyBuffer[bufferIndex]);
        // ... (mapToLogScale, 计算 y, p.lineTo) ...
    }
    return p;
};

// 3. 执行波形绘制 (逻辑保持不变，但现在会使用动态缩放后的 Path)
// ... (绘制 Detector, Synth, Output) ...
【3.3】高精度“狙击镜”覆盖层（EnvelopeView.cpp::paint）在paint最后一个瞄准镜，其形状根据释放时间动态变化。C++￼// EnvelopeView.cpp::paint() (最后)

if (isDynamicZoomActive.load())
{
    // 绘制狙击镜 ()。形状根据 Release 时间变化，并有最小圆形约束。
    
    // 使用非线性映射 (对数) 来确定视觉上的“张开程度”
    // 假设 Release 范围从 10ms 到 5000ms (请根据实际参数范围调整)
    float minLog = std::log(10.0f);
    float maxLog = std::log(5000.0f);
    float currentLog = std::log(juce::jlimit(10.0f, 5000.0f, releaseMs));
    
    // 映射到 0.0 (最窄/圆形) 到 1.0 (最宽/平直)
    float openness = (currentLog - minLog) / (maxLog - minLog);

    // 定义最小宽度 (接近圆形) 和最大宽度
    float minWidth = height * 0.8f;
    float maxWidth = width * 0.95f;
    
    float scopeWidth = juce::jmap(openness, minWidth, maxWidth);
    float scopeX = (width - scopeWidth) / 2.0f;
    
    // 绘制两个弧形 ()
    juce::Path scopePath;
    float arcHeight = height * 0.8f;
    float arcY = (height - arcHeight) / 2.0f;

    // 使用 juce::Path::addArc 绘制。通过调整起始和结束角度来模拟开合。
    // 角度调整范围 (例如从 0.55π/1.45π 到 0.8π/1.2π)
    float angleSweep = juce::jmap(openness, 0.9f * juce::MathConstants<float>::pi, 0.4f * juce::MathConstants<float>::pi);
    
    // 左侧弧 (
    float startAngleL = juce::MathConstants<float>::pi - angleSweep / 2.0f;
    float endAngleL = juce::MathConstants<float>::pi + angleSweep / 2.0f;
    // 我们需要一个足够大的半径来让弧线出现在正确的位置，这里使用 addArc 的边界框来定位
    scopePath.addArc(scopeX - scopeWidth/2, arcY, scopeWidth * 2, arcHeight, endAngleL, startAngleL, true);


    // 右侧弧 )
    float startAngleR = -angleSweep / 2.0f;
    float endAngleR = angleSweep / 2.0f;
    scopePath.addArc(scopeX + width - scopeWidth/2, arcY, scopeWidth * 2, arcHeight, startAngleR, endAngleR, true);


    // 绘制风格
    g.setColour(referenceColour.withAlpha(0.6f)); // 使用 T/C 线颜色
    g.strokePath(scopePath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
￼四模块：DSP逻辑【4.1】 自动增益逻辑审查用户反馈开启Auto-Gain后信号声音变大。• 任务：审查PluginProcessor.cpp中的自动增益实现逻辑。确定其计算方式（最高/RMS？补偿系数？）。• 报告要求：请在完成本次迭代修改后，在回复中简要说明自动增益的当前工作逻辑，以及你对此正确性的判断。暂时不要代码，只进行分析和报告。【4.2】Soft Clip实现（精确限制器）实现一个精确的限制器：-0.01dB限制，自动补偿0.01dB。• 实现位置：在processBlock的最后结束（产出前）。C++￼// PluginProcessor.cpp (processBlock 末尾)

// ⚠️ 获取 Soft Clip 开关状态 (请使用正确的参数 ID/逻辑)
bool softClipEnabled = /* ... */; 

if (softClipEnabled)
{
    // 目标电平阈值: -0.01 dB
    static const float threshold = juce::Decibels::decibelsToGain(-0.01f); // 约 0.99885
    // 补偿增益 (Makeup Gain): +0.01 dB
    static const float makeupGain = juce::Decibels::decibelsToGain(0.01f); // 约 1.00115

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float sample = channelData[i];

            // 1. 应用补偿增益 (将信号推入限制器)
            sample *= makeupGain;

            // 2. 应用精确限制 (Hard Clipping)
            // 确保输出精确控制在 -0.01dB (threshold)
            sample = juce::jlimit(-threshold, threshold, sample);

            channelData[i] = sample;
        }
    }
}
￼5. 执行要求请立即执行所有模块的。重点关注模块一（精确计时滚动归零）和模块三（动态时间缩放与狙击镜高精度逻辑修改）的实现，这是本次迭代的核心挑战。完成后请使用新的版本号格式规范（V18.6 - YYYYMMDD.02）提交到GitHub。
