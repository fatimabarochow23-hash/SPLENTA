/*
  ==============================================================================
    EnvelopeView.cpp
    Dynamic Envelope Network Visualization Component
    Created: 2025-11-27
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

    // Calculate X step for each history point
    const float xStep = width / (float)(historySize - 1);

    // === Draw Threshold and Ceiling Reference Lines ===
    // Get parameter values (in dB)
    float thresholdDB = processor.apvts->getRawParameterValue("THRESHOLD")->load();
    float ceilingDB = processor.apvts->getRawParameterValue("CEILING")->load();

    // Convert dB to linear amplitude
    float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDB);
    float ceilingLinear = juce::Decibels::decibelsToGain(ceilingDB);

    // Apply logarithmic mapping
    float thresholdVisual = mapToLogScale(thresholdLinear);
    float ceilingVisual = mapToLogScale(ceilingLinear);

    // Calculate Y coordinates (flip because Y increases downward)
    float thresholdY = height * (1.0f - thresholdVisual);
    float ceilingY = height * (1.0f - ceilingVisual);

    // Draw Ceiling line (solid, brighter)
    g.setColour(referenceColour);
    g.drawLine(0.0f, ceilingY, width, ceilingY, 1.5f);
    g.setFont(12.0f);
    g.drawText("C", 5, (int)ceilingY - 14, 20, 12, juce::Justification::left, false);

    // Draw Threshold line (dashed)
    g.setColour(referenceColour.withAlpha(0.7f));
    juce::Line<float> threshLine(0.0f, thresholdY, width, thresholdY);
    float dashLengths[] = { 4.0f, 4.0f };
    g.drawDashedLine(threshLine, dashLengths, 2, 1.0f);
    g.drawText("T", 5, (int)thresholdY + 2, 20, 12, juce::Justification::left, false);

    // === Draw Detector Envelope (bottom layer, thicker) ===
    {
        juce::Path detectorPath;
        bool started = false;

        for (int i = 0; i < historySize; ++i)
        {
            float x = i * xStep;
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

        for (int i = 0; i < historySize; ++i)
        {
            float x = i * xStep;
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
        for (int i = 0; i < historySize; ++i)
        {
            float x = i * xStep;
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

        for (int i = 0; i < historySize; ++i)
        {
            float x = i * xStep;
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
        // Rotate left by numToRead positions, then insert new data at the end
        std::rotate(historyBuffer.begin(),
                   historyBuffer.begin() + numToRead,
                   historyBuffer.end());

        // Copy new data to the end of history buffer
        std::copy(tempBuffer.begin(), tempBuffer.end(),
                 historyBuffer.end() - numToRead);
    }
}
