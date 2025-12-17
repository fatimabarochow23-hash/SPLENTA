/*
  ==============================================================================
    EnvelopeView.cpp (SPLENTA V18.6 - 20251212.01)
    Extended Temporal Window & Dynamic Zoom Visualization
  ==============================================================================
*/

#include "EnvelopeView.h"
#include "PluginProcessor.h"

EnvelopeView::EnvelopeView(NewProjectAudioProcessor& p)
    : processor(p)
{
    // Initialize history buffer with zeros
    for (auto& point : historyBuffer)
    {
        point.detector = 0.0f;
        point.synthesizer = 0.0f;
        point.output = 0.0f;
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

    // Fill background
    g.fillAll(backgroundColour);

    // === Oscilloscope-Style Grid Lines (Pro-C2 inspired) ===
    g.setFont(10.0f);
    const float gridDBLevels[] = {-12.0f, -24.0f, -36.0f, -48.0f};

    for (float db : gridDBLevels)
    {
        float linearValue = juce::Decibels::decibelsToGain(db);
        float logValue = mapToLogScale(linearValue);
        float y = height * (1.0f - logValue);

        // Draw grid line
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawLine(0.0f, y, width, y, 1.0f);

        // Draw dB label on right side
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawText(juce::String((int)db), (int)width - 30, (int)y - 8, 25, 16, juce::Justification::right, false);
    }

    // === Dynamic Zoom Scaling (Module 3.2) ===
    int displayStartIndex = 0;
    int displayEndIndex = historySize;
    float xStep = width / (float)(historySize - 1);

    if (processor.isDynamicZoomActive.load())
    {
        // Get Release parameter and calculate zoom range
        float releaseMS = processor.apvts->getRawParameterValue("DET_REL")->load();

        // Calculate display window based on Release time (with 1.2x context)
        double sampleRate = processor.atomicSampleRate.load();
        int updateRate = 128;
        float releaseSamples = (releaseMS / 1000.0f) * sampleRate;
        int releasePoints = static_cast<int>(releaseSamples / updateRate);

        // Apply 1.2x context multiplier
        int displayPoints = static_cast<int>(releasePoints * 1.2f);
        displayPoints = juce::jlimit(100, historySize, displayPoints);

        // Show most recent displayPoints
        displayStartIndex = historySize - displayPoints;
        displayEndIndex = historySize;

        // Recalculate xStep for zoomed view
        xStep = width / (float)(displayPoints - 1);
    }

    // === Draw Threshold and Ceiling Reference Lines (Simplified) ===
    float thresholdDB = processor.apvts->getRawParameterValue("THRESHOLD")->load();
    float ceilingDB = processor.apvts->getRawParameterValue("CEILING")->load();

    float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDB);
    float ceilingLinear = juce::Decibels::decibelsToGain(ceilingDB);

    float thresholdVisual = mapToLogScale(thresholdLinear);
    float ceilingVisual = mapToLogScale(ceilingLinear);

    float thresholdY = height * (1.0f - thresholdVisual);
    float ceilingY = height * (1.0f - ceilingVisual);

    // Draw Ceiling line (solid red, as per standard)
    g.setColour(juce::Colour(0xFFFF4444));  // Red
    g.drawLine(0.0f, ceilingY, width, ceilingY, 1.0f);
    g.setFont(10.0f);
    g.setColour(juce::Colour(0xFFFF4444).withAlpha(0.8f));
    g.drawText("CEIL", 5, (int)ceilingY - 12, 35, 10, juce::Justification::left, false);

    // Draw Threshold line (dashed white, as per standard)
    g.setColour(juce::Colour(0xFFFFFFFF));  // White
    juce::Path thresholdPath, dashedPath;
    thresholdPath.startNewSubPath(0.0f, thresholdY);
    thresholdPath.lineTo(width, thresholdY);
    float dashLengths[] = {4.0f, 4.0f};  // Dash pattern: 4px on, 4px off
    juce::PathStrokeType(1.0f).createDashedStroke(dashedPath, thresholdPath, dashLengths, 2);
    g.strokePath(dashedPath, juce::PathStrokeType(1.0f));
    g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.8f));
    g.drawText("THR", 5, (int)thresholdY + 2, 30, 10, juce::Justification::left, false);

    // === Draw Synthesizer Envelope (main waveform with gradient fill) ===
    {
        juce::Path synthPath;
        synthPath.startNewSubPath(0.0f, height);

        for (int i = displayStartIndex; i < displayEndIndex; ++i)
        {
            float x = (i - displayStartIndex) * xStep;
            float logValue = mapToLogScale(historyBuffer[i].synthesizer);
            float y = height * (1.0f - logValue);
            synthPath.lineTo(x, y);
        }

        synthPath.lineTo(width, height);
        synthPath.closeSubPath();

        // Pro-C2 style gradient fill (top to bottom, high alpha to low alpha)
        juce::ColourGradient gradient(
            synthColour.withAlpha(0.5f),  // Top: 50% alpha
            0.0f, 0.0f,
            synthColour.withAlpha(0.05f), // Bottom: 5% alpha
            0.0f, height,
            false
        );
        g.setGradientFill(gradient);
        g.fillPath(synthPath);

        // Stroke the top edge (clean single line)
        juce::Path synthStrokePath;
        bool started = false;
        for (int i = displayStartIndex; i < displayEndIndex; ++i)
        {
            float x = (i - displayStartIndex) * xStep;
            float logValue = mapToLogScale(historyBuffer[i].synthesizer);
            float y = height * (1.0f - logValue);

            if (!started)
            {
                synthStrokePath.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                synthStrokePath.lineTo(x, y);
            }
        }

        g.setColour(synthColour);
        g.strokePath(synthStrokePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
    }
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
    // Read new data points from processor's FIFO
    int numReady = processor.envelopeFifo.getNumReady();

    if (numReady > 0)
    {
        // Limit the number of points to read to avoid buffer overflow
        int numToRead = juce::jmin(numReady, historySize);

        // Temporary buffer for reading from FIFO
        std::vector<EnvelopeDataPoint> tempBuffer(numToRead);

        // Read from FIFO
        int start1, size1, start2, size2;
        processor.envelopeFifo.prepareToRead(numToRead, start1, size1, start2, size2);

        if (size1 > 0)
            std::copy(processor.envelopeBuffer.begin() + start1,
                     processor.envelopeBuffer.begin() + start1 + size1,
                     tempBuffer.begin());

        if (size2 > 0)
            std::copy(processor.envelopeBuffer.begin() + start2,
                     processor.envelopeBuffer.begin() + start2 + size2,
                     tempBuffer.begin() + size1);

        processor.envelopeFifo.finishedRead(size1 + size2);

        // Efficient scrolling using std::rotate
        std::rotate(historyBuffer.begin(),
                   historyBuffer.begin() + numToRead,
                   historyBuffer.end());

        // Copy new data to the end of history buffer
        std::copy(tempBuffer.begin(), tempBuffer.end(),
                 historyBuffer.end() - numToRead);

        // Update time tracking
        lastUpdateTime = juce::Time::getCurrentTime();
        hasReceivedData = true;
    }
    else if (hasReceivedData && !processor.isFrozen.load())
    {
        // Scroll-to-silence logic: inject zero-value points when audio stops
        auto currentTime = juce::Time::getCurrentTime();
        auto elapsedMs = currentTime.toMilliseconds() - lastUpdateTime.toMilliseconds();

        // Calculate how many zero points to inject based on elapsed time
        double sampleRate = processor.atomicSampleRate.load();
        int updateRate = 128;  // envUpdateRate
        double pointsPerSecond = sampleRate / updateRate;
        int zeroPointsToInject = static_cast<int>((elapsedMs / 1000.0) * pointsPerSecond);

        // Limit to reasonable amount
        zeroPointsToInject = juce::jmin(zeroPointsToInject, 10);

        if (zeroPointsToInject > 0)
        {
            // Create zero-value data points
            EnvelopeDataPoint zeroPoint;
            zeroPoint.detector = 0.0f;
            zeroPoint.synthesizer = 0.0f;
            zeroPoint.output = 0.0f;

            // Inject zero points to simulate natural fadeout
            std::rotate(historyBuffer.begin(),
                       historyBuffer.begin() + zeroPointsToInject,
                       historyBuffer.end());

            // Fill with zeros
            for (int i = 0; i < zeroPointsToInject; ++i)
                historyBuffer[historySize - zeroPointsToInject + i] = zeroPoint;

            lastUpdateTime = currentTime;
        }
    }
}

void EnvelopeView::setThemeColors(juce::Colour accent, juce::Colour panel)
{
    backgroundColour = panel;
    synthColour = accent;
    detectorColour = accent.darker(0.4f).withSaturation(0.6f);
    outputColour = accent.brighter(0.3f);
    referenceColour = accent.withRotatedHue(0.5f).withSaturation(0.7f);
}
