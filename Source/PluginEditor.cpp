/*
  ==============================================================================
    PluginEditor.cpp (SPLENTA V18.5 - 20251127.01)
    Dynamic Envelope Visualization System
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), envelopeView(p)
{
    setupKnob(threshSlider, "THRESHOLD", threshAtt, " dB");
    setupKnob(ceilingSlider,"CEILING",   ceilingAtt," dB");
    setupKnob(relSlider,    "DET_REL",   relAtt,    " ms");
    setupKnob(waitSlider,   "WAIT_MS",   waitAtt,   " ms");
    setupKnob(freqSlider,   "F_FREQ",    freqAtt,   " Hz");
    setupKnob(qSlider,      "F_Q",       qAtt,      "");

    setupKnob(startFreqSlider, "START_FREQ", startFreqAtt, " Hz");
    setupKnob(peakFreqSlider,  "PEAK_FREQ",  peakFreqAtt,  " Hz");
    setupKnob(satSlider,       "SATURATION", satAtt,       " %");
    setupKnob(noiseSlider,     "NOISE_MIX",  noiseAtt,     " %");

    setupKnob(pAttSlider, "P_ATT", pAttAtt, " ms");
    setupKnob(pDecSlider, "P_DEC", pDecAtt, " ms");
    setupKnob(aAttSlider, "A_ATT", aAttAtt, " ms");
    setupKnob(aDecSlider, "A_DEC", aDecAtt, " ms");

    setupKnob(duckSlider,    "DUCKING",  duckAtt,    " dB");
    setupKnob(duckAttSlider, "D_ATT",    duckAttAtt, " ms");
    setupKnob(duckDecSlider, "D_DEC",    duckDecAtt, " ms");
    setupKnob(wetSlider,     "WET_GAIN", wetAtt,     " dB");
    setupKnob(drySlider,     "DRY_MIX",  dryAtt,     " %");
    setupKnob(mixSlider,     "MIX",      mixAtt,     " %");

    addAndMakeVisible(shapeBox);
    shapeBox.addItemList( {"Sine", "Triangle", "Square"}, 1 );
    shapeAtt.reset(new ComboBoxAttachment(*audioProcessor.apvts, "SHAPE", shapeBox));

    addAndMakeVisible(auditionButton);
    auditionButton.setClickingTogglesState(true);
    auditionButton.setAlpha(0.0f);
    auditionAtt.reset(new ButtonAttachment(*audioProcessor.apvts, "AUDITION", auditionButton));

    addAndMakeVisible(themeBox);
    themeBox.addItemList( {"RX Classic", "ARC Rust", "Pro Purple", "Astro Grey", "Acid Volt"}, 1 );
    themeAtt.reset(new ComboBoxAttachment(*audioProcessor.apvts, "THEME", themeBox));
    themeBox.onChange = [this] { updateColors(); repaint(); };
    
    addAndMakeVisible(presetBox);
    presetBox.setText("Preset");
    presetBox.addSectionHeading("Realistic");
    presetBox.addItem("Gunshot", 1);
    presetBox.addItem("Sword", 2);
    presetBox.addSeparator();
    presetBox.addSectionHeading("Sci-Fi");
    presetBox.addItem("Laser", 3);
    presetBox.addItem("Pulse", 4);
    presetBox.onChange = [this] { if(presetBox.getSelectedId()>0) audioProcessor.loadPreset(presetBox.getSelectedId()); };

    addAndMakeVisible(agmButton);
    agmButton.setClickingTogglesState(true);
    agmAtt.reset(new ButtonAttachment(*audioProcessor.apvts, "AGM_MODE", agmButton));
    
    addAndMakeVisible(clipButton);
    clipButton.setClickingTogglesState(true);
    clipAtt.reset(new ButtonAttachment(*audioProcessor.apvts, "SOFT_CLIP", clipButton));
    
    addAndMakeVisible(expandButton);
    expandButton.setButtonText("");
    expandButton.setAlpha(0.0f);
    expandButton.onClick = [this] { showFFT = !showFFT; resized(); repaint(); };

    addAndMakeVisible(envelopeView);

    setSize (960, 620);
    startTimerHz(60);
    updateColors();
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor() { stopTimer(); }

void NewProjectAudioProcessorEditor::setupKnob(juce::Slider& slider, const juce::String& id, std::unique_ptr<SliderAttachment>& attachment, const juce::String& suffix)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.9f));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    attachment.reset(new SliderAttachment(*audioProcessor.apvts, id, slider));
}

void NewProjectAudioProcessorEditor::updateColors()
{
    int themeID = themeBox.getSelectedItemIndex();
    if (themeID < 0) themeID = 1;
    juce::Colour c_accent, c_panel;

    switch (themeID) {
        case 0: c_accent=juce::Colour(0xff42a5f5); c_panel=juce::Colour(0xff22262e); break;
        case 1: c_accent=juce::Colour(0xffff7722); c_panel=juce::Colour(0xff110b09); break;
        case 2: c_accent=juce::Colour(0xff9d55ff); c_panel=juce::Colour(0xff222229); break;
        case 3: c_accent=juce::Colour(0xffffaa00); c_panel=juce::Colour(0xff383838); break;
        case 4: c_accent=juce::Colour(0xffccff00); c_panel=juce::Colour(0xff101210); break;
    }

    // Update EnvelopeView theme colors dynamically
    envelopeView.setThemeColors(c_accent, c_panel);

    auto apply = [&](juce::Slider& s) {
        s.setColour(juce::Slider::thumbColourId, c_accent);
        s.setColour(juce::Slider::rotarySliderFillColourId, c_accent);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, c_panel.brighter(0.2f));
    };
    apply(threshSlider); apply(ceilingSlider); apply(relSlider); apply(waitSlider); apply(freqSlider); apply(qSlider);
    apply(startFreqSlider); apply(peakFreqSlider); apply(satSlider); apply(noiseSlider);
    apply(pAttSlider); apply(pDecSlider); apply(aAttSlider); apply(aDecSlider);
    apply(duckSlider); apply(duckAttSlider); apply(duckDecSlider); apply(wetSlider); apply(drySlider); apply(mixSlider);

    auto applyBtn = [&](juce::TextButton& b) {
        b.setColour(juce::TextButton::buttonOnColourId, c_accent);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    };
    applyBtn(agmButton); applyBtn(clipButton); applyBtn(expandButton);
}

void NewProjectAudioProcessorEditor::mouseMove(const juce::MouseEvent& e) {
    if (dividerArea.contains(e.getPosition()) && showFFT)
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void NewProjectAudioProcessorEditor::mouseDown(const juce::MouseEvent& e) {
    if (dividerArea.contains(e.getPosition()) && showFFT)
        currentDragAction = Splitter;
}

void NewProjectAudioProcessorEditor::mouseUp(const juce::MouseEvent&) {
    currentDragAction = None;
}

void NewProjectAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) {
    if (currentDragAction == Splitter && showFFT) {
        float newRatio = (float)(e.getPosition().x - 10) / 880.0f;
        splitRatio = juce::jlimit(0.1f, 0.9f, newRatio);
        resized();
        repaint();
    }
}

void NewProjectAudioProcessorEditor::drawPixelArt(juce::Graphics& g, int startX, int startY, int scale, juce::Colour color, int type)
{
    std::vector<std::string> icon;
    switch (type) {
        case 0: icon={"00111100","01111110","10100101","11111111","11111111","01011010","01000010","01100110"}; break;
        case 1: icon={"00011000","00111100","01100110","11000011","11011011","10111101","00111100","00011000"}; break;
        case 2: icon={"00111100","01111110","11011011","11111111","01111110","10011001","10100101","01000010"}; break;
        case 3: icon={"00011000","00111100","00111100","01111110","01100110","01100110","11100111","10111101"}; break;
        case 4: icon={"00011000","01111110","11111111","11011011","11111111","11011011","01111110","00100100"}; break;
    }
    g.setColour(color);
    for(int r=0; r<8; ++r) for(int c=0; c<8; ++c) if(icon[r][c]=='1') g.fillRect(startX+c*scale, startY+r*scale, scale, scale);
}

void NewProjectAudioProcessorEditor::drawPixelHeadphone(juce::Graphics& g, int x, int y, int scale, juce::Colour c)
{
    std::vector<std::string> icon = { "0011111100", "0100000010", "0100000010", "1110000111", "1110000111", "1110000111", "0110000110", "0000000000" };
    g.setColour(c);
    for(int r=0; r<8; ++r) for(int c=0; c<10; ++c) if(icon[r][c]=='1') g.fillRect(x+c*scale, y+r*scale, scale, scale);
}

void NewProjectAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.nextFFTBlockReady) {
        audioProcessor.forwardFFT.performFrequencyOnlyForwardTransform (audioProcessor.fftData.data());
        audioProcessor.nextFFTBlockReady = false;
        repaint();
    } else {
        repaint();
    }
}

void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::Colour c_bg, c_panel, c_accent, c_text;
    int themeID = themeBox.getSelectedItemIndex();
    if (themeID < 0) themeID = 1;
    switch (themeID) {
        case 0: c_bg=juce::Colour(0xff181b20); c_panel=juce::Colour(0xff22262e); c_accent=juce::Colour(0xff42a5f5); break;
        case 1: c_bg=juce::Colour(0xff1a1210); c_panel=juce::Colour(0xff110b09); c_accent=juce::Colour(0xffff7722); break;
        case 2: c_bg=juce::Colour(0xff18181c); c_panel=juce::Colour(0xff222229); c_accent=juce::Colour(0xff9d55ff); break;
        case 3: c_bg=juce::Colour(0xff2a2a2a); c_panel=juce::Colour(0xff383838); c_accent=juce::Colour(0xffffaa00); break;
        case 4: c_bg=juce::Colour(0xff050505); c_panel=juce::Colour(0xff101210); c_accent=juce::Colour(0xffccff00); break;
    }
    c_text = juce::Colours::white;
    g.fillAll (c_bg);

    // Areas
    int totalW = 960 - 20; int startX = 10; int topY = 30; int height = 200;
    int splitX = startX + (int)(totalW * (showFFT ? splitRatio : 1.0f));

    envelopeArea = juce::Rectangle<int>(startX, topY, splitX - startX, height);
    dividerArea = juce::Rectangle<int>(splitX, topY, showFFT ? 4 : 0, height);
    fftArea = juce::Rectangle<int>(splitX + 4, topY, (startX + totalW) - (splitX + 4), height);

    // Envelope View
    envelopeView.setBounds(envelopeArea);
    g.setColour (c_panel); g.fillRect (envelopeArea);
    g.setColour (c_accent.withAlpha(0.2f)); g.drawRect (envelopeArea);

    // FFT
    if (showFFT) {
        g.setColour (c_panel); g.fillRect (fftArea);
        g.setColour (c_accent.withAlpha(0.2f)); g.drawRect (fftArea);
        g.saveState(); g.reduceClipRegion(fftArea);
        auto mapFreqToX = [&](float freq) -> float {
            float minF = 20.0f; float maxF = 20000.0f;
            float normX = (std::log10(freq) - std::log10(minF)) / (std::log10(maxF) - std::log10(minF));
            return fftArea.getX() + (fftArea.getWidth() * normX);
        };
        g.setFont(10.0f);
        std::vector<float> majors = { 100.0f, 1000.0f, 10000.0f };
        g.setColour(c_accent.withAlpha(0.4f));
        for (float f : majors) {
             float x = mapFreqToX(f);
             if(x>fftArea.getX()) {
                 g.drawVerticalLine((int)x, (float)fftArea.getY(), (float)fftArea.getBottom());
                 g.setColour(c_text.withAlpha(0.9f));
                 g.drawText(f>=1000?juce::String(f/1000)+"k":juce::String((int)f), (int)x+2, fftArea.getY()+2, 30, 12, juce::Justification::left);
             }
        }
        g.setColour(c_accent.withAlpha(0.8f));
        juce::Path fftPath; fftPath.startNewSubPath(fftArea.getX(), fftArea.getBottom());
        auto& fftData = audioProcessor.fftData;
        for (int i = 1; i < 1024; ++i) {
            float binFreq = i * (48000.0f / 2048.0f); float x = mapFreqToX(binFreq);
            float db = juce::Decibels::gainToDecibels(fftData[i]) - juce::Decibels::gainToDecibels(2048.0f) + 10.0f;
            float y = fftArea.getBottom() - (juce::jmap(db, -100.0f, 0.0f, 0.0f, 1.0f) * fftArea.getHeight());
            if(x>=fftArea.getX()) fftPath.lineTo(x, y);
        }
        fftPath.lineTo(fftArea.getRight(), fftArea.getBottom()); fftPath.closeSubPath();
        g.setColour(c_accent.withAlpha(0.3f)); g.fillPath(fftPath);
        g.setColour(c_accent); g.strokePath(fftPath, juce::PathStrokeType(1.5f));
        g.restoreState();

        g.setColour(c_accent.withAlpha(0.5f)); g.fillRect(dividerArea);
        if (dividerArea.contains(getMouseXYRelative())) g.setColour(juce::Colours::white);

        int cx = expandButton.getX() + expandButton.getWidth()/2;
        int cy = expandButton.getY() + expandButton.getHeight()/2;

        // Circular expand button background
        g.setColour(c_panel.brighter(0.3f));
        g.fillEllipse(expandButton.getX(), expandButton.getY(), 24, 24);

        g.setColour(c_text);
        juce::Path arrow;
        arrow.startNewSubPath(cx-3, cy-5); arrow.lineTo(cx+2, cy); arrow.lineTo(cx-3, cy+5); // >
        g.strokePath(arrow, juce::PathStrokeType(2.0f));
    } else {
        // === Sniper Scope Visualization (When FFT Collapsed) ===
        int scopeX = envelopeArea.getRight() + 20;
        int scopeY = envelopeArea.getY() + envelopeArea.getHeight() / 2;

        // Get Release parameter value (DET_REL)
        float releaseMS = audioProcessor.apvts->getRawParameterValue("DET_REL")->load();

        // Map Release time to bracket gap width (1ms -> 10px, 500ms -> 100px)
        float gapWidth = juce::jmap(releaseMS, 1.0f, 500.0f, 10.0f, 100.0f);

        g.setColour(c_accent);
        g.setFont(18.0f);

        // Draw parametric bracket visualization
        int bracketHeight = 40;
        int leftBracketX = scopeX - (int)(gapWidth / 2);
        int rightBracketX = scopeX + (int)(gapWidth / 2);

        // Left bracket: (
        juce::Path leftBracket;
        leftBracket.startNewSubPath(leftBracketX + 8, scopeY - bracketHeight/2);
        leftBracket.quadraticTo(leftBracketX, scopeY, leftBracketX + 8, scopeY + bracketHeight/2);
        g.strokePath(leftBracket, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));

        // Right bracket: )
        juce::Path rightBracket;
        rightBracket.startNewSubPath(rightBracketX - 8, scopeY - bracketHeight/2);
        rightBracket.quadraticTo(rightBracketX, scopeY, rightBracketX - 8, scopeY + bracketHeight/2);
        g.strokePath(rightBracket, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));

        // Display Release value
        g.setFont(12.0f);
        g.setColour(c_text.withAlpha(0.8f));
        g.drawText("REL: " + juce::String((int)releaseMS) + "ms",
                   scopeX - 40, scopeY + bracketHeight/2 + 10, 80, 20,
                   juce::Justification::centred);

        int cx = expandButton.getX() + expandButton.getWidth()/2;
        int cy = expandButton.getY() + expandButton.getHeight()/2;

        // Circular expand button background
        g.setColour(c_panel.brighter(0.3f));
        g.fillEllipse(expandButton.getX(), expandButton.getY(), 24, 24);

        g.setColour(c_text);
        juce::Path arrow;
        arrow.startNewSubPath(cx+2, cy-5); arrow.lineTo(cx-3, cy); arrow.lineTo(cx+2, cy+5); // <
        g.strokePath(arrow, juce::PathStrokeType(2.0f));
    }

    if (audioProcessor.isTriggeredUI) {
        int iconX = showFFT ? (fftArea.getRight() - 50) : (envelopeArea.getRight() - 50);
        drawPixelArt(g, iconX, 45, 4, c_accent, themeID);
    }
    
    juce::Colour earColor = auditionButton.getToggleState() ? c_accent : c_text.withAlpha(0.2f);
    drawPixelHeadphone(g, auditionButton.getX(), auditionButton.getY() + 2, 2, earColor);
    g.setColour(earColor); g.setFont(12.0f);
    g.drawText("Listen", auditionButton.getX() + 25, auditionButton.getY(), 50, 20, juce::Justification::left);

    // Labels
    g.setColour (c_accent); g.setFont (juce::FontOptions(14.0f, juce::Font::bold));
    int startY = 250; int colW = 220;
    g.drawText ("// DETECTOR", 20, startY, 150, 20, juce::Justification::left);
    g.drawText ("// GENERATOR", 20 + colW, startY, 150, 20, juce::Justification::left);
    g.drawText ("// ENVELOPE",  20 + colW*2, startY, 150, 20, juce::Justification::left);
    g.drawText ("// OUTPUT",    20 + colW*3, startY, 150, 20, juce::Justification::left);

    g.setColour (c_text.withAlpha(0.8f)); g.setFont (juce::FontOptions(12.0f));
    auto drawLabel = [&](juce::Slider& s, const juce::String& name) { g.drawText (name, s.getX(), s.getY() - 15, s.getWidth(), 15, juce::Justification::centred); };
    drawLabel(threshSlider, "Thresh"); drawLabel(ceilingSlider, "Ceiling"); drawLabel(relSlider, "Rel"); drawLabel(waitSlider, "Wait"); drawLabel(freqSlider, "Freq"); drawLabel(qSlider, "Q");
    drawLabel(startFreqSlider, "Start"); drawLabel(peakFreqSlider, "Peak"); drawLabel(satSlider, "Sat"); drawLabel(noiseSlider, "Noise");
    drawLabel(pAttSlider, "P.Att"); drawLabel(pDecSlider, "P.Dec"); drawLabel(aAttSlider, "A.Att"); drawLabel(aDecSlider, "A.Dec");
    drawLabel(duckSlider, "Duck"); drawLabel(duckAttSlider, "D.Att"); drawLabel(duckDecSlider, "D.Dec"); drawLabel(wetSlider, "Wet"); drawLabel(drySlider, "Dry"); drawLabel(mixSlider, "Mix");
    g.setColour (c_accent.brighter(0.2f)); g.setFont (juce::FontOptions ("Arial", 24.0f, juce::Font::bold | juce::Font::italic)); g.drawText ("SPLENTA", 800, 550, 120, 30, juce::Justification::right);
    g.setFont (juce::FontOptions(10.0f)); g.setColour (c_text.withAlpha(0.8f)); g.drawText ("AUDIO TOOLS", 800, 575, 100, 10, juce::Justification::right);
    g.setColour (c_accent); g.fillRect (835, 572, 65, 2);
    g.setColour(c_text.withAlpha(0.5f)); g.drawText("Theme:", 740, 5, 50, 20, juce::Justification::right); g.drawText("Preset:", 80, 5, 50, 20, juce::Justification::left);
}

void NewProjectAudioProcessorEditor::resized()
{
    int startY = 290; int colW = 220; int knobSize = 65; int gap = 85;
    themeBox.setBounds(790, 5, 120, 20);
    presetBox.setBounds(130, 5, 150, 20);
    int totalW = 960 - 20;
    int splitX = 10 + (int)(totalW * (showFFT ? splitRatio : 1.0f));
    expandButton.setBounds(splitX - 12, 98, 24, 24); // Circular 24x24px
    agmButton.setBounds(860, 250, 80, 25); clipButton.setBounds(860, 290, 80, 25);
    threshSlider.setBounds (20, startY, knobSize, knobSize); ceilingSlider.setBounds(100, startY, knobSize, knobSize); relSlider.setBounds(20, startY + gap, knobSize, knobSize); waitSlider.setBounds(100, startY + gap, knobSize, knobSize); freqSlider.setBounds(20, startY + gap*2, knobSize, knobSize); qSlider.setBounds(100, startY + gap*2, knobSize, knobSize); auditionButton.setBounds(20, startY + gap*3, 40, 25);
    startFreqSlider.setBounds (20 + colW, startY, knobSize, knobSize); peakFreqSlider.setBounds(100 + colW, startY, knobSize, knobSize); satSlider.setBounds(20 + colW, startY + gap, knobSize, knobSize); noiseSlider.setBounds(100 + colW, startY + gap, knobSize, knobSize); shapeBox.setBounds(20 + colW, startY + gap*2, 150, 25);
    pAttSlider.setBounds (20 + colW*2, startY, knobSize, knobSize); pDecSlider.setBounds(100 + colW*2, startY, knobSize, knobSize); aAttSlider.setBounds(20 + colW*2, startY + gap, knobSize, knobSize); aDecSlider.setBounds(100 + colW*2, startY + gap, knobSize, knobSize);
    duckSlider.setBounds (20 + colW*3, startY, knobSize, knobSize); mixSlider.setBounds(100 + colW*3, startY, knobSize, knobSize); duckAttSlider.setBounds(20 + colW*3, startY + gap, knobSize, knobSize); duckDecSlider.setBounds(100 + colW*3, startY + gap, knobSize, knobSize); wetSlider.setBounds(20 + colW*3, startY + gap*2, knobSize, knobSize); drySlider.setBounds(100 + colW*3, startY + gap*2, knobSize, knobSize);
}
