/*
  ==============================================================================
    PluginEditor.cpp (SPLENTA V18.7 - 20251216.10)
    Batch 06: Custom Controls (Waveform & Split-Toggle) + Web-Style Header
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), envelopeView(p),
      waveformSelector(*p.apvts), splitToggle(*p.apvts)
{
    // Custom components (Batch 06) - Add BEFORE sliders to ensure on top
    addAndMakeVisible(waveformSelector);
    waveformSelector.setAlwaysOnTop(true);
    addAndMakeVisible(splitToggle);
    splitToggle.setAlwaysOnTop(true);

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

    // Web-Style Header (Batch 06 Task 3)
    // SPLENTA logo removed per user request

    addAndMakeVisible(presetNameLabel);
    presetNameLabel.setText("-- No Preset --", juce::dontSendNotification);
    presetNameLabel.setFont(stealthLnF.getMonospaceFont(12.0f));
    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));

    addAndMakeVisible(saveButton);
    saveButton.setButtonText("SAVE");
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    saveButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));
    saveButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    saveButton.onClick = [this] {
        // Placeholder: save preset logic
        presetNameLabel.setText("Custom Preset", juce::dontSendNotification);
    };

    addAndMakeVisible(loadButton);
    loadButton.setButtonText("LOAD");
    loadButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));
    loadButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    loadButton.onClick = [this] {
        // Create popup menu with presets
        juce::PopupMenu menu;
        menu.addSectionHeader("Realistic");
        menu.addItem(1, "Gunshot");
        menu.addItem(2, "Sword");
        menu.addSeparator();
        menu.addSectionHeader("Sci-Fi");
        menu.addItem(3, "Laser");
        menu.addItem(4, "Pulse");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&loadButton),
                           [this](int result) {
                               if (result > 0) {
                                   audioProcessor.loadPreset(result);
                                   // Update preset name
                                   juce::String presetName;
                                   switch(result) {
                                       case 1: presetName = "Gunshot"; break;
                                       case 2: presetName = "Sword"; break;
                                       case 3: presetName = "Laser"; break;
                                       case 4: presetName = "Pulse"; break;
                                   }
                                   presetNameLabel.setText(presetName, juce::dontSendNotification);
                               }
                           });
    };

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

    // Update custom components (Batch 06)
    waveformSelector.setPalette(palette);
    splitToggle.setPalette(palette);

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

void NewProjectAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title, bool isActive)
{
    // Get current theme
    int themeIndex = (int)audioProcessor.apvts->getRawParameterValue("THEME")->load();
    auto palette = ThemePalette::getPaletteByIndex(themeIndex);

    const int headerHeight = 26;
    auto headerArea = bounds.removeFromTop(headerHeight);
    auto contentArea = bounds;

    // Panel background (ultra-subtle white overlay)
    g.setColour(juce::Colours::white.withAlpha(0.03f));
    g.fillRect(contentArea);

    // Header background
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRect(headerArea);

    // Header bottom border
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawLine(headerArea.getX(), headerArea.getBottom(),
               headerArea.getRight(), headerArea.getBottom(), 1.0f);

    // Corner accents (L-shaped decorations)
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    const int cornerLen = 6;
    auto fullBounds = headerArea.getUnion(contentArea);

    // Top-left
    g.drawLine(fullBounds.getX(), fullBounds.getY(),
               fullBounds.getX() + cornerLen, fullBounds.getY(), 1.0f);
    g.drawLine(fullBounds.getX(), fullBounds.getY(),
               fullBounds.getX(), fullBounds.getY() + cornerLen, 1.0f);

    // Top-right
    g.drawLine(fullBounds.getRight() - cornerLen, fullBounds.getY(),
               fullBounds.getRight(), fullBounds.getY(), 1.0f);
    g.drawLine(fullBounds.getRight(), fullBounds.getY(),
               fullBounds.getRight(), fullBounds.getY() + cornerLen, 1.0f);

    // Bottom-left
    g.drawLine(fullBounds.getX(), fullBounds.getBottom() - cornerLen,
               fullBounds.getX(), fullBounds.getBottom(), 1.0f);
    g.drawLine(fullBounds.getX(), fullBounds.getBottom(),
               fullBounds.getX() + cornerLen, fullBounds.getBottom(), 1.0f);

    // Bottom-right
    g.drawLine(fullBounds.getRight(), fullBounds.getBottom() - cornerLen,
               fullBounds.getRight(), fullBounds.getBottom(), 1.0f);
    g.drawLine(fullBounds.getRight() - cornerLen, fullBounds.getBottom(),
               fullBounds.getRight(), fullBounds.getBottom(), 1.0f);

    // Title text
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(title.toUpperCase(), headerArea.reduced(8, 0),
               juce::Justification::centredLeft);

    // Status dot
    float dotSize = 4.0f;
    float dotX = headerArea.getRight() - 12.0f;
    float dotY = headerArea.getCentreY() - dotSize / 2.0f;
    g.setColour(isActive ? palette.accent : juce::Colours::white.withAlpha(0.15f));
    g.fillEllipse(dotX, dotY, dotSize, dotSize);
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

    // Update trigger particle scatter effect in Energy Topology
    energyTopology.setTriggerState(audioProcessor.isTriggeredUI);

    repaint();
}

void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Get theme colors
    int themeIndex = (int)audioProcessor.apvts->getRawParameterValue("THEME")->load();
    auto palette = ThemePalette::getPaletteByIndex(themeIndex);

    juce::Colour c_accent = palette.accent;
    juce::Colour c_text = juce::Colours::white;

    // Global background (theme-aware)
    g.fillAll(palette.background());

    // Define 4 panel areas (with spacing)
    const int margin = 10;
    const int spacing = 10;
    const int topPanelHeight = 230;
    const int bottomPanelHeight = 340;

    // Top row: Input Detector (Envelope) + Energy Topology
    detectorPanel = juce::Rectangle<int>(margin, margin, 470, topPanelHeight);
    topologyPanel = juce::Rectangle<int>(detectorPanel.getRight() + spacing, margin, 460, topPanelHeight);

    // Bottom row: Enforcer Core + Master Output
    enforcerPanel = juce::Rectangle<int>(margin, detectorPanel.getBottom() + spacing, 660, bottomPanelHeight);
    outputPanel = juce::Rectangle<int>(enforcerPanel.getRight() + spacing, detectorPanel.getBottom() + spacing, 280, bottomPanelHeight);

    // Draw panels with industrial styling
    drawPanel(g, detectorPanel, "INPUT DETECTOR", audioProcessor.isTriggeredUI);
    drawPanel(g, topologyPanel, "ENERGY TOPOLOGY", true);
    drawPanel(g, enforcerPanel, "ENFORCER CORE", true);
    drawPanel(g, outputPanel, "MASTER OUTPUT", true);

    // Position visualizers inside their panels (adjust for header)
    const int headerHeight = 26;
    envelopeArea = detectorPanel.withTrimmedTop(headerHeight).reduced(4);
    topologyArea = topologyPanel.withTrimmedTop(headerHeight).reduced(4);
    envelopeView.setBounds(envelopeArea);

    // Trigger icon (inside detector panel)
    if (audioProcessor.isTriggeredUI) {
        int iconX = envelopeArea.getRight() - 40;
        int iconY = envelopeArea.getY() + 10;
        drawPixelArt(g, iconX, iconY, 4, c_accent, themeIndex);
    }

    // Audition button (headphone icon)
    juce::Colour earColor = auditionButton.getToggleState() ? c_accent : c_text.withAlpha(0.2f);
    drawPixelHeadphone(g, auditionButton.getX(), auditionButton.getY() + 2, 2, earColor);
    g.setColour(earColor); g.setFont(12.0f);
    g.drawText("Listen", auditionButton.getX() + 25, auditionButton.getY(), 50, 20, juce::Justification::left);

    // Section labels (below knobs, at bottom of panels)
    g.setColour(c_accent); g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    int labelY = 595;  // Bottom of knob area
    int colW = 220;
    g.drawText("// DETECTOR", 20, labelY, 150, 20, juce::Justification::left);
    g.drawText("// GENERATOR", 20 + colW, labelY, 150, 20, juce::Justification::left);
    g.drawText("// ENVELOPE", 20 + colW*2, labelY, 150, 20, juce::Justification::left);

    // Output section label (at bottom of output panel)
    g.drawText("// OUTPUT", outputPanel.getX() + 20, labelY, 150, 20, juce::Justification::left);

    // Knob labels
    g.setColour(c_text.withAlpha(0.8f)); g.setFont(juce::FontOptions(12.0f));
    auto drawLabel = [&](juce::Slider& s, const juce::String& name) {
        g.drawText(name, s.getX(), s.getY() - 15, s.getWidth(), 15, juce::Justification::centred);
    };
    drawLabel(threshSlider, "Thresh"); drawLabel(ceilingSlider, "Ceiling"); drawLabel(relSlider, "Rel");
    drawLabel(waitSlider, "Wait"); drawLabel(freqSlider, "Freq"); drawLabel(qSlider, "Q");
    drawLabel(startFreqSlider, "Start"); drawLabel(peakFreqSlider, "Peak"); drawLabel(satSlider, "Sat"); drawLabel(noiseSlider, "Noise");
    drawLabel(pAttSlider, "P.Att"); drawLabel(pDecSlider, "P.Dec"); drawLabel(aAttSlider, "A.Att"); drawLabel(aDecSlider, "A.Dec");
    drawLabel(duckSlider, "Duck"); drawLabel(duckAttSlider, "D.Att"); drawLabel(duckDecSlider, "D.Dec");
    drawLabel(wetSlider, "Wet"); drawLabel(drySlider, "Dry"); drawLabel(mixSlider, "Mix");

    // Footer branding removed (per user request - causes lag even with static white colors)
}

void NewProjectAudioProcessorEditor::resized()
{
    int startY = 290; int colW = 220; int knobSize = 60; int gap = 92;  // Reduced knob size, increased spacing

    // Web-Style Header (top bar) - SAVE/LOAD right of TOPOLOGY
    saveButton.setBounds(650, 5, 60, 24);
    loadButton.setBounds(720, 5, 60, 24);
    presetNameLabel.setBounds(350, 5, 200, 24);
    themeSelector.setBounds(790, 5, 110, 24);

    // Custom components (Batch 06)
    waveformSelector.setBounds(20 + colW, startY + gap*2, 150, 28);  // Replace shapeBox
    splitToggle.setBounds(850, startY, 75, 75);  // Output panel right side, no overlap

    // Energy Topology bounds (matches paint() panel calculation)
    const int margin = 10;
    const int spacing = 10;
    const int topPanelHeight = 230;
    const int headerHeight = 26;

    juce::Rectangle<int> topologyPanelArea(490, margin, 460, topPanelHeight);
    energyTopology.setBounds(topologyPanelArea.withTrimmedTop(headerHeight).reduced(4));

    threshSlider.setBounds (20, startY, knobSize, knobSize); ceilingSlider.setBounds(100, startY, knobSize, knobSize); relSlider.setBounds(20, startY + gap, knobSize, knobSize); waitSlider.setBounds(100, startY + gap, knobSize, knobSize); freqSlider.setBounds(20, startY + gap*2, knobSize, knobSize); qSlider.setBounds(100, startY + gap*2, knobSize, knobSize); auditionButton.setBounds(20, startY + gap*3, 40, 25);
    startFreqSlider.setBounds (20 + colW, startY, knobSize, knobSize); peakFreqSlider.setBounds(100 + colW, startY, knobSize, knobSize); satSlider.setBounds(20 + colW, startY + gap, knobSize, knobSize); noiseSlider.setBounds(100 + colW, startY + gap, knobSize, knobSize);
    pAttSlider.setBounds (20 + colW*2, startY, knobSize, knobSize); pDecSlider.setBounds(100 + colW*2, startY, knobSize, knobSize); aAttSlider.setBounds(20 + colW*2, startY + gap, knobSize, knobSize); aDecSlider.setBounds(100 + colW*2, startY + gap, knobSize, knobSize);
    duckSlider.setBounds (20 + colW*3, startY, knobSize, knobSize); mixSlider.setBounds(100 + colW*3, startY, knobSize, knobSize); duckAttSlider.setBounds(20 + colW*3, startY + gap, knobSize, knobSize); duckDecSlider.setBounds(100 + colW*3, startY + gap, knobSize, knobSize); wetSlider.setBounds(20 + colW*3, startY + gap*2, knobSize, knobSize); drySlider.setBounds(100 + colW*3, startY + gap*2, knobSize, knobSize);
}
