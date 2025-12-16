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
    juce::Path thresholdPath;
    thresholdPath.startNewSubPath(0.0f, thresholdY);
    thresholdPath.lineTo(width, thresholdY);
    float dashLengths[] = {4.0f, 4.0f};  // Dash pattern: 4px on, 4px off
    g.strokePath(thresholdPath, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt),
                 juce::AffineTransform(), dashLengths, 2);
    g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.8f));
    g.drawText("THR", 5, (int)thresholdY + 2, 30, 10, juce::Justification::left, false);

    // === Draw Detector Envelope (bottom layer, thicker) ===
    {
        juce::Path detectorPath;
        bool started = false;

        for (int i = displayStartIndex; i < displayEndIndex; ++i)
        {
            float x = (i - displayStartIndex) * xStep;
            float logValue = mapToLogScale(historyBuffer[i].detector);
            float y = height * (1.0f - logValue);

            if (!started)
            {
                detectorPath.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                detectorPath.lineTo(x, y);
            }
        }

        g.setColour(detectorColour);
        g.strokePath(detectorPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));
    }

    // === Draw Synthesizer Envelope (middle layer, filled + stroked) ===
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

        // Fill with semi-transparent orange
        g.setColour(synthColour.withAlpha(0.4f));
        g.fillPath(synthPath);

        // Stroke the top edge
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
        g.strokePath(synthStrokePath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));
    }

    // === Draw Output Envelope (top layer, thinner, bright) ===
    {
        juce::Path outputPath;
        bool started = false;

        for (int i = displayStartIndex; i < displayEndIndex; ++i)
        {
            float x = (i - displayStartIndex) * xStep;
            float logValue = mapToLogScale(historyBuffer[i].output);
            float y = height * (1.0f - logValue);

            if (!started)
            {
                outputPath.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                outputPath.lineTo(x, y);
            }
        }

        g.setColour(outputColour);
        g.strokePath(outputPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));
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
