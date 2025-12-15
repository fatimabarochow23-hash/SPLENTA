/*
  ==============================================================================
    PluginEditor.cpp (SPLENTA V18.6 - 20251216.06)
    Batch 04: Energy Topology (Mobius Visualizer Integration)
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

    // Initialize ThemeSelector
    addAndMakeVisible(themeSelector);
    int currentThemeIndex = (int)audioProcessor.apvts->getRawParameterValue("THEME")->load();
    themeSelector.setSelectedIndex(currentThemeIndex);
    themeSelector.onThemeChanged = [this](int index) {
        audioProcessor.setParameterValue("THEME", (float)index);
        updateColors();
    };

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

    addAndMakeVisible(envelopeView);
    addAndMakeVisible(energyTopology);

    // Apply custom LookAndFeel
    setLookAndFeel(&stealthLnF);

    setSize (960, 620);
    startTimerHz(60);
    updateColors();
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void NewProjectAudioProcessorEditor::setupKnob(juce::Slider& slider, const juce::String& id, std::unique_ptr<SliderAttachment>& attachment, const juce::String& suffix)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.9f));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    // Set rotary angle range to match Web version (-145° to +145°)
    slider.setRotaryParameters(juce::degreesToRadians(-145.0f), juce::degreesToRadians(145.0f), true);

    attachment.reset(new SliderAttachment(*audioProcessor.apvts, id, slider));

    // Initialize alpha to 0 for show-on-interaction behavior
    knobTextAlpha[&slider] = 0.0f;
}

void NewProjectAudioProcessorEditor::updateColors()
{
    // Read theme index from THEME parameter
    int themeIndex = (int)audioProcessor.apvts->getRawParameterValue("THEME")->load();
    auto palette = ThemePalette::getPaletteByIndex(themeIndex);

    // Update custom LookAndFeel with new palette
    stealthLnF.setPalette(palette);

    // Update ThemeSelector
    themeSelector.setSelectedIndex(themeIndex);

    // Update EnvelopeView colors
    envelopeView.setThemeColors(palette.accent, palette.panel900);

    // Update EnergyTopology colors
    energyTopology.setPalette(palette);

    // Apply colors to sliders (use accent with alpha from map for text box color)
    auto apply = [&](juce::Slider& s) {
        s.setColour(juce::Slider::thumbColourId, palette.accent);
        s.setColour(juce::Slider::rotarySliderFillColourId, palette.accent);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, palette.panel900.brighter(0.2f));

        // Apply text color with current alpha
        float alpha = knobTextAlpha[&s];
        s.setColour(juce::Slider::textBoxTextColourId, palette.accent.withAlpha(alpha));
    };
    apply(threshSlider); apply(ceilingSlider); apply(relSlider); apply(waitSlider); apply(freqSlider); apply(qSlider);
    apply(startFreqSlider); apply(peakFreqSlider); apply(satSlider); apply(noiseSlider);
    apply(pAttSlider); apply(pDecSlider); apply(aAttSlider); apply(aDecSlider);
    apply(duckSlider); apply(duckAttSlider); apply(duckDecSlider); apply(wetSlider); apply(drySlider); apply(mixSlider);

    // Apply colors to buttons
    auto applyBtn = [&](juce::TextButton& b) {
        b.setColour(juce::TextButton::buttonOnColourId, palette.accent);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    };
    applyBtn(agmButton); applyBtn(clipButton);
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
    // Theme change detection (for host automation / state restore)
    int currentThemeIndex = juce::roundToInt(audioProcessor.apvts->getRawParameterValue("THEME")->load());
    currentThemeIndex = juce::jlimit(0, 4, currentThemeIndex);

    if (currentThemeIndex != lastThemeIndex)
    {
        lastThemeIndex = currentThemeIndex;
        updateColors();
    }

    // Knob value show-on-interaction with fade effect
    auto palette = ThemePalette::getPaletteByIndex(currentThemeIndex);
    bool needsColorUpdate = false;

    auto updateKnobAlpha = [&](juce::Slider& slider) {
        float targetAlpha = slider.isMouseButtonDown() ? 1.0f : 0.0f;
        float& currentAlpha = knobTextAlpha[&slider];
        float newAlpha = currentAlpha + (targetAlpha - currentAlpha) * 0.2f;

        if (std::abs(newAlpha - currentAlpha) > 0.001f) {
            currentAlpha = newAlpha;
            slider.setColour(juce::Slider::textBoxTextColourId, palette.accent.withAlpha(currentAlpha));
            needsColorUpdate = true;
        }
    };

    updateKnobAlpha(threshSlider); updateKnobAlpha(ceilingSlider); updateKnobAlpha(relSlider);
    updateKnobAlpha(waitSlider); updateKnobAlpha(freqSlider); updateKnobAlpha(qSlider);
    updateKnobAlpha(startFreqSlider); updateKnobAlpha(peakFreqSlider); updateKnobAlpha(satSlider); updateKnobAlpha(noiseSlider);
    updateKnobAlpha(pAttSlider); updateKnobAlpha(pDecSlider); updateKnobAlpha(aAttSlider); updateKnobAlpha(aDecSlider);
    updateKnobAlpha(duckSlider); updateKnobAlpha(duckAttSlider); updateKnobAlpha(duckDecSlider);
    updateKnobAlpha(wetSlider); updateKnobAlpha(drySlider); updateKnobAlpha(mixSlider);

    repaint();
}

void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Get theme colors
    int themeIndex = (int)audioProcessor.apvts->getRawParameterValue("THEME")->load();
    auto palette = ThemePalette::getPaletteByIndex(themeIndex);

    juce::Colour c_bg = palette.bg950;
    juce::Colour c_panel = palette.panel900;
    juce::Colour c_accent = palette.accent;
    juce::Colour c_glow = palette.glow;
    juce::Colour c_text = juce::Colours::white;

    g.fillAll (c_bg);

    // Top visualization areas
    int startX = 10; int topY = 30; int height = 200;
    int envelopeWidth = 470;  // Half of original ~940
    int topologyWidth = 460;   // Remaining width

    envelopeArea = juce::Rectangle<int>(startX, topY, envelopeWidth, height);
    topologyArea = juce::Rectangle<int>(startX + envelopeWidth + 10, topY, topologyWidth, height);

    // Envelope View panel
    envelopeView.setBounds(envelopeArea);
    g.setColour (c_panel); g.fillRect (envelopeArea);
    g.setColour (c_accent.withAlpha(0.2f)); g.drawRect (envelopeArea);

    // Energy Topology panel background
    g.setColour (c_panel); g.fillRect (topologyArea);
    g.setColour (c_accent.withAlpha(0.2f)); g.drawRect (topologyArea);

    // Trigger icon
    if (audioProcessor.isTriggeredUI) {
        int iconX = envelopeArea.getRight() - 50;
        drawPixelArt(g, iconX, 45, 4, c_accent, themeIndex);
    }

    // Audition button (headphone icon)
    juce::Colour earColor = auditionButton.getToggleState() ? c_accent : c_text.withAlpha(0.2f);
    drawPixelHeadphone(g, auditionButton.getX(), auditionButton.getY() + 2, 2, earColor);
    g.setColour(earColor); g.setFont(12.0f);
    g.drawText("Listen", auditionButton.getX() + 25, auditionButton.getY(), 50, 20, juce::Justification::left);

    // Section labels
    g.setColour (c_accent); g.setFont (juce::FontOptions(14.0f, juce::Font::bold));
    int startY = 250; int colW = 220;
    g.drawText ("// DETECTOR", 20, startY, 150, 20, juce::Justification::left);
    g.drawText ("// GENERATOR", 20 + colW, startY, 150, 20, juce::Justification::left);
    g.drawText ("// ENVELOPE",  20 + colW*2, startY, 150, 20, juce::Justification::left);
    g.drawText ("// OUTPUT",    20 + colW*3, startY, 150, 20, juce::Justification::left);

    // Knob labels
    g.setColour (c_text.withAlpha(0.8f)); g.setFont (juce::FontOptions(12.0f));
    auto drawLabel = [&](juce::Slider& s, const juce::String& name) { g.drawText (name, s.getX(), s.getY() - 15, s.getWidth(), 15, juce::Justification::centred); };
    drawLabel(threshSlider, "Thresh"); drawLabel(ceilingSlider, "Ceiling"); drawLabel(relSlider, "Rel"); drawLabel(waitSlider, "Wait"); drawLabel(freqSlider, "Freq"); drawLabel(qSlider, "Q");
    drawLabel(startFreqSlider, "Start"); drawLabel(peakFreqSlider, "Peak"); drawLabel(satSlider, "Sat"); drawLabel(noiseSlider, "Noise");
    drawLabel(pAttSlider, "P.Att"); drawLabel(pDecSlider, "P.Dec"); drawLabel(aAttSlider, "A.Att"); drawLabel(aDecSlider, "A.Dec");
    drawLabel(duckSlider, "Duck"); drawLabel(duckAttSlider, "D.Att"); drawLabel(duckDecSlider, "D.Dec"); drawLabel(wetSlider, "Wet"); drawLabel(drySlider, "Dry"); drawLabel(mixSlider, "Mix");

    // Branding
    g.setColour (c_accent.brighter(0.2f)); g.setFont (juce::FontOptions ("Arial", 24.0f, juce::Font::bold | juce::Font::italic)); g.drawText ("SPLENTA", 800, 550, 120, 30, juce::Justification::right);
    g.setFont (juce::FontOptions(10.0f)); g.setColour (c_text.withAlpha(0.8f)); g.drawText ("AUDIO TOOLS", 800, 575, 100, 10, juce::Justification::right);
    g.setColour (c_accent); g.fillRect (835, 572, 65, 2);
    g.setColour(c_text.withAlpha(0.5f)); g.drawText("Preset:", 80, 5, 50, 20, juce::Justification::left);
}

void NewProjectAudioProcessorEditor::resized()
{
    int startY = 290; int colW = 220; int knobSize = 65; int gap = 85;
    themeSelector.setBounds(790, 5, 150, 24);
    presetBox.setBounds(130, 5, 150, 20);
    agmButton.setBounds(860, 250, 80, 25); clipButton.setBounds(860, 290, 80, 25);

    // Top visualization areas (match paint() calculation)
    int startX = 10; int topY = 30; int height = 200;
    int envelopeWidth = 470;
    int topologyWidth = 460;
    energyTopology.setBounds(startX + envelopeWidth + 10, topY, topologyWidth, height);

    threshSlider.setBounds (20, startY, knobSize, knobSize); ceilingSlider.setBounds(100, startY, knobSize, knobSize); relSlider.setBounds(20, startY + gap, knobSize, knobSize); waitSlider.setBounds(100, startY + gap, knobSize, knobSize); freqSlider.setBounds(20, startY + gap*2, knobSize, knobSize); qSlider.setBounds(100, startY + gap*2, knobSize, knobSize); auditionButton.setBounds(20, startY + gap*3, 40, 25);
    startFreqSlider.setBounds (20 + colW, startY, knobSize, knobSize); peakFreqSlider.setBounds(100 + colW, startY, knobSize, knobSize); satSlider.setBounds(20 + colW, startY + gap, knobSize, knobSize); noiseSlider.setBounds(100 + colW, startY + gap, knobSize, knobSize); shapeBox.setBounds(20 + colW, startY + gap*2, 150, 25);
    pAttSlider.setBounds (20 + colW*2, startY, knobSize, knobSize); pDecSlider.setBounds(100 + colW*2, startY, knobSize, knobSize); aAttSlider.setBounds(20 + colW*2, startY + gap, knobSize, knobSize); aDecSlider.setBounds(100 + colW*2, startY + gap, knobSize, knobSize);
    duckSlider.setBounds (20 + colW*3, startY, knobSize, knobSize); mixSlider.setBounds(100 + colW*3, startY, knobSize, knobSize); duckAttSlider.setBounds(20 + colW*3, startY + gap, knobSize, knobSize); duckDecSlider.setBounds(100 + colW*3, startY + gap, knobSize, knobSize); wetSlider.setBounds(20 + colW*3, startY + gap*2, knobSize, knobSize); drySlider.setBounds(100 + colW*3, startY + gap*2, knobSize, knobSize);
}
