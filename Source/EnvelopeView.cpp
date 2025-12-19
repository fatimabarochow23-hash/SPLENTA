/*
  ==============================================================================
    EnvelopeView.cpp (SPLENTA V19.3 - 20251219.02)
    Frozen Trigger Waveform Display (ShaperBox 3 Style)
  ==============================================================================
*/

#include "EnvelopeView.h"
#include "PluginProcessor.h"

EnvelopeView::EnvelopeView(NewProjectAudioProcessor& p)
    : processor(p)
{
    // Initialize frozen waveform with zeros
    for (auto& point : frozenWaveform)
    {
        point.detectorInput = 0.0f;
        point.synthOutput = 0.0f;
        point.finalOutput = 0.0f;
    }

    // Start 60Hz timer
    startTimerHz(60);
}

EnvelopeView::~EnvelopeView()
{
    stopTimer();
}

void EnvelopeView::paint(juce::Graphics& g)
{
    const float width = (float)getWidth();
    const float height = (float)getHeight();

    // Dark background (ShaperBox 3 style)
    g.fillAll(backgroundColour);

    // Grid lines for amplitude reference (unipolar: 0% at bottom, 100% at top)
    // Draw reference lines at 25%, 50%, 75% height
    for (float level : {0.25f, 0.5f, 0.75f})
    {
        float y = height * level;
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.drawLine(0.0f, y, width, y, 1.0f);
    }

    // Draw threshold line (horizontal, dashed)
    float thresholdDB = processor.apvts->getRawParameterValue("THRESHOLD")->load();
    float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDB);

    // Apply auto-scale factor to match waveform scaling
    // This ensures threshold line aligns with the actual trigger level in the displayed waveform
    float scaledThreshold = thresholdLinear * autoScaleFactor;

    // Use unipolar mapping for threshold (0 = bottom, 1+ = top)
    float thresholdY = mapThresholdToVisualY(scaledThreshold, height);

    // Draw Ceiling line (solid red reference line)
    float ceilingDB = processor.apvts->getRawParameterValue("CEILING")->load();
    float ceilingLinear = juce::Decibels::decibelsToGain(ceilingDB);

    // Apply auto-scale factor to ceiling as well
    float scaledCeiling = ceilingLinear * autoScaleFactor;

    // Use unipolar mapping for ceiling
    float ceilingY = mapThresholdToVisualY(scaledCeiling, height);

    // === Detector Range Highlight (between Threshold and Ceiling) ===
    // This shows the active detection zone
    if (ceilingY < thresholdY)  // Make sure ceiling is above threshold
    {
        float rangeHeight = thresholdY - ceilingY;
        g.setColour(thresholdColour.withAlpha(0.08f));  // Subtle glow
        g.fillRect(0.0f, ceilingY, width, rangeHeight);
    }

    // Draw threshold line (dashed)
    g.setColour(thresholdColour.withAlpha(0.6f));
    juce::Path thresholdPath, dashedPath;
    thresholdPath.startNewSubPath(0.0f, thresholdY);
    thresholdPath.lineTo(width, thresholdY);
    float dashLengths[] = {6.0f, 4.0f};
    juce::PathStrokeType(1.5f).createDashedStroke(dashedPath, thresholdPath, dashLengths, 2);
    g.strokePath(dashedPath, juce::PathStrokeType(1.5f));

    // Threshold label
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(thresholdColour.withAlpha(0.8f));
    g.drawText("THR", 5, (int)thresholdY - 12, 30, 10, juce::Justification::left);

    // Draw ceiling line (solid red)
    g.setColour(juce::Colour(0xFFFF4444));  // Red
    g.drawLine(0.0f, ceilingY, width, ceilingY, 1.0f);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xFFFF4444).withAlpha(0.8f));
    g.drawText("CEIL", 5, (int)ceilingY - 12, 35, 10, juce::Justification::left);

    // If no valid snapshot, show "Waiting for trigger..." message
    if (!hasValidSnapshot)
    {
        g.setFont(juce::FontOptions(14.0f));
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawText("Waiting for trigger...", 0, (int)height / 2 - 10, (int)width, 20, juce::Justification::centred);
        return;
    }

    // === Smooth Waveform Rendering (Oscilloscope Style) ===
    // Use RMS-based decimation for smooth continuous lines
    int screenWidth = (int)width;

    // Find actual valid sample count (dynamic during scrolling)
    int validSamples = 0;
    for (int i = waveformSize - 1; i >= 0; --i)
    {
        if (frozenWaveform[i].detectorInput != 0.0f || frozenWaveform[i].finalOutput != 0.0f)
        {
            validSamples = i + 1;
            break;
        }
    }

    if (validSamples == 0)
    {
        g.setFont(juce::FontOptions(14.0f));
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawText("Waiting for trigger...", 0, (int)height / 2 - 10, (int)width, 20, juce::Justification::centred);
        return;
    }

    int samplesPerPixel = std::max(1, validSamples / screenWidth);

    // Build smooth amplitude envelope paths (unipolar, like DAW waveform overview)
    // Use envelope follower with attack/release smoothing for clean contour
    juce::Path detectorPath, outputPath;
    bool detectorPathStarted = false;
    bool outputPathStarted = false;

    // Envelope follower state (peak detection with smooth decay)
    float detEnvelope = 0.0f;
    float outEnvelope = 0.0f;

    // Store envelope values for highlight fill rendering
    std::vector<float> outputEnvelopeValues;
    outputEnvelopeValues.reserve(screenWidth);

    // Smoothing coefficients (adjust for visual smoothness)
    const float attackCoeff = 0.1f;   // Fast response to peaks
    const float releaseCoeff = 0.995f; // Slow decay for smooth contour

    for (int pixel = 0; pixel < screenWidth; ++pixel)
    {
        int startSample = pixel * samplesPerPixel;
        int endSample = std::min(startSample + samplesPerPixel, validSamples);

        if (startSample >= validSamples) break;

        // Find peak value in this pixel range (for envelope follower)
        float detPeak = 0.0f;
        float outPeak = 0.0f;

        for (int i = startSample; i < endSample; ++i)
        {
            float detAbs = std::abs(frozenWaveform[i].detectorInput);
            float outAbs = std::abs(frozenWaveform[i].finalOutput);

            if (detAbs > detPeak) detPeak = detAbs;
            if (outAbs > outPeak) outPeak = outAbs;
        }

        // Envelope follower: fast attack, slow release (like analog peak detector)
        if (detPeak > detEnvelope)
            detEnvelope = detPeak * attackCoeff + detEnvelope * (1.0f - attackCoeff);
        else
            detEnvelope = detEnvelope * releaseCoeff;

        if (outPeak > outEnvelope)
            outEnvelope = outPeak * attackCoeff + outEnvelope * (1.0f - attackCoeff);
        else
            outEnvelope = outEnvelope * releaseCoeff;

        // Store output envelope for highlight fill
        outputEnvelopeValues.push_back(outEnvelope);

        float x = (float)pixel;

        // Use unipolar mapping: 0 = bottom, peak = top (like Enforcer/DAW waveform)
        float detY = mapThresholdToVisualY(detEnvelope, height);
        float outY = mapThresholdToVisualY(outEnvelope, height);

        // Build continuous path
        if (!detectorPathStarted)
        {
            detectorPath.startNewSubPath(x, detY);
            detectorPathStarted = true;
        }
        else
        {
            detectorPath.lineTo(x, detY);
        }

        if (!outputPathStarted)
        {
            outputPath.startNewSubPath(x, outY);
            outputPathStarted = true;
        }
        else
        {
            outputPath.lineTo(x, outY);
        }
    }

    // Layer 1: Detector Input (50% alpha, dimmer - background layer)
    g.setColour(detectorColour);  // 50% alpha
    g.strokePath(detectorPath, juce::PathStrokeType(1.5f));

    // Layer 2: Output waveform (100% alpha - main layer)
    g.setColour(outputColour);  // 100% alpha (full brightness)
    g.strokePath(outputPath, juce::PathStrokeType(2.0f));

    // === X-Axis Time Highlight (Enforcer style) ===
    // Fill the area under the output waveform curve during enhancement period
    // This creates a visual "integral" effect showing low-frequency enhancement duration

    if ((isScrolling || validSamples > 0) && !outputEnvelopeValues.empty())
    {
        // Calculate enhancement duration in samples
        double sampleRate = processor.atomicSampleRate.load();
        float aAttMs = processor.apvts->getRawParameterValue("A_ATT")->load();
        float aDecMs = processor.apvts->getRawParameterValue("A_DEC")->load();
        int preTriggerSamples = (int)(sampleRate * 0.010);
        int enhancementDuration = (int)(sampleRate * (aAttMs + aDecMs) / 1000.0);

        // Calculate screen positions for highlight region
        int enhancementStartSample = preTriggerSamples;  // Start after pre-trigger
        int enhancementEndSample = std::min(enhancementStartSample + enhancementDuration, validSamples);

        // Convert sample positions to screen X coordinates
        float pixelsPerSample = screenWidth / (float)validSamples;
        int highlightStartPixel = (int)(enhancementStartSample * pixelsPerSample);
        int highlightEndPixel = (int)(enhancementEndSample * pixelsPerSample);

        if (highlightStartPixel < highlightEndPixel && highlightEndPixel <= (int)outputEnvelopeValues.size())
        {
            // Build filled path for the highlighted region (under the smoothed envelope curve)
            juce::Path filledPath;
            bool pathStarted = false;

            // Trace the smoothed output envelope curve in the highlight region
            for (int pixel = highlightStartPixel; pixel <= highlightEndPixel && pixel < (int)outputEnvelopeValues.size(); ++pixel)
            {
                float x = (float)pixel;
                float envelopeValue = outputEnvelopeValues[pixel];
                float y = mapThresholdToVisualY(envelopeValue, height);

                if (!pathStarted)
                {
                    // Start from bottom-left corner of highlight region
                    filledPath.startNewSubPath(x, height);
                    filledPath.lineTo(x, y);
                    pathStarted = true;
                }
                else
                {
                    filledPath.lineTo(x, y);
                }
            }

            // Close the path by going back to baseline
            if (pathStarted)
            {
                float lastX = (float)std::min(highlightEndPixel, (int)outputEnvelopeValues.size() - 1);
                filledPath.lineTo(lastX, height);  // Down to bottom-right
                filledPath.closeSubPath();  // Back to start

                // Fill the area under the curve with semi-transparent color
                g.setColour(outputColour.withAlpha(0.25f));
                g.fillPath(filledPath);

                // Redraw output waveform in this region with brighter stroke
                g.saveState();
                g.reduceClipRegion(highlightStartPixel, 0, highlightEndPixel - highlightStartPixel, (int)height);
                g.setColour(outputColour.brighter(0.3f));
                g.strokePath(outputPath, juce::PathStrokeType(2.5f));
                g.restoreState();
            }
        }
    }

    // Legend (top-right corner) - simplified, two layers only
    int legendX = (int)width - 120;
    int legendY = 10;
    int lineHeight = 14;

    g.setFont(juce::FontOptions(9.0f));

    // Detector (50% alpha)
    g.setColour(detectorColour);
    g.fillRect(legendX, legendY, 12, 2);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText("Detector", legendX + 16, legendY - 6, 90, 12, juce::Justification::left);

    // Output (100% alpha, brightest)
    g.setColour(outputColour);
    g.fillRect(legendX, legendY + lineHeight, 12, 2);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText("Output", legendX + 16, legendY + lineHeight - 6, 90, 12, juce::Justification::left);
}

void EnvelopeView::resized()
{
    // No dynamic layout needed
}

void EnvelopeView::timerCallback()
{
    updateFromProcessor();
    repaint();
}

void EnvelopeView::updateFromProcessor()
{
    // Detect trigger state change (rising edge)
    bool currentTriggerState = processor.isTriggeredUI.load();

    if (currentTriggerState && !lastTriggerState)
    {
        // === NEW TRIGGER DETECTED ===
        // Record trigger moment and start real-time scrolling
        triggerScopePos = processor.dualScopeWritePos.load();
        triggerTime = 0;
        isScrolling = true;

        // Calculate maximum display window based on amplitude envelope
        double sampleRate = processor.atomicSampleRate.load();
        float aAttMs = processor.apvts->getRawParameterValue("A_ATT")->load();
        float aDecMs = processor.apvts->getRawParameterValue("A_DEC")->load();
        int preTriggerSamples = (int)(sampleRate * 0.010);  // 10ms pre-trigger
        int aAttSamples = (int)(sampleRate * aAttMs / 1000.0);
        int aDecSamples = (int)(sampleRate * aDecMs / 1000.0);

        maxDisplaySamples = preTriggerSamples + aAttSamples + aDecSamples;
        maxDisplaySamples = std::min(maxDisplaySamples, waveformSize);

        // Clear waveform buffer
        for (auto& point : frozenWaveform)
        {
            point.detectorInput = 0.0f;
            point.synthOutput = 0.0f;
            point.finalOutput = 0.0f;
        }

        hasValidSnapshot = true;
    }

    // === REAL-TIME SCROLLING UPDATE ===
    if (isScrolling)
    {
        int currentScopePos = processor.dualScopeWritePos.load();
        int scopeSize = processor.dualScopeBufferSize;

        // Calculate how many samples elapsed since trigger
        int elapsedSamples = (currentScopePos - triggerScopePos + scopeSize) % scopeSize;

        // Include pre-trigger (10ms before trigger)
        double sampleRate = processor.atomicSampleRate.load();
        int preTriggerSamples = (int)(sampleRate * 0.010);
        int totalSamples = elapsedSamples + preTriggerSamples;

        // Clamp to max display window
        if (totalSamples > maxDisplaySamples)
        {
            totalSamples = maxDisplaySamples;
            isScrolling = false;  // Stop scrolling when complete
        }

        // Read from two independent buffers
        const float* detectorData = processor.detectorScopeBuffer.getReadPointer(0);
        const float* outputData = processor.outputScopeBuffer.getReadPointer(0);

        // === Auto-scale: find peak in current visible range ===
        float maxPeak = 0.0f;
        for (int i = 0; i < totalSamples; ++i)
        {
            int readIndex = (triggerScopePos - preTriggerSamples + i + scopeSize) % scopeSize;
            float detSample = std::abs(detectorData[readIndex]);
            float outSample = std::abs(outputData[readIndex]);
            if (detSample > maxPeak) maxPeak = detSample;
            if (outSample > maxPeak) maxPeak = outSample;
        }

        // Calculate auto-scale factor
        const float targetPeak = 0.9f;
        if (maxPeak > 0.0001f)
            autoScaleFactor = targetPeak / maxPeak;
        else
            autoScaleFactor = 1.0f;

        // === Read and display waveform from trigger to current ===
        for (int i = 0; i < totalSamples; ++i)
        {
            int readIndex = (triggerScopePos - preTriggerSamples + i + scopeSize) % scopeSize;

            frozenWaveform[i].detectorInput = detectorData[readIndex] * autoScaleFactor;
            frozenWaveform[i].synthOutput = 0.0f;
            frozenWaveform[i].finalOutput = outputData[readIndex] * autoScaleFactor;
        }

        // Clear remaining samples
        for (int i = totalSamples; i < waveformSize; ++i)
        {
            frozenWaveform[i].detectorInput = 0.0f;
            frozenWaveform[i].synthOutput = 0.0f;
            frozenWaveform[i].finalOutput = 0.0f;
        }
    }

    lastTriggerState = currentTriggerState;
}

void EnvelopeView::setThemeColors(juce::Colour accent, juce::Colour panel)
{
    // Two layers with theme accent at different alpha for distinction
    detectorColour = accent.withAlpha(0.5f);           // 50% - background layer
    outputColour = accent;                              // 100% - main layer on top
    thresholdColour = juce::Colours::white;            // White threshold line
    triggerHighlight = accent.withAlpha(0.15f);        // Subtle highlight
    backgroundColour = panel.darker(0.8f);             // Dark background

    repaint();
}
