/*
  ==============================================================================
    PluginEditor.cpp (SPLENTA V19.5 - 20251223.01)
    A/B Compare with 3D Pyramid Animation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), envelopeView(p),
      waveformSelector(*p.apvts), splitToggle(*p.apvts), powerButton(*p.apvts), colorControl(*p.apvts), midiToggle(*p.apvts), retriggerModeSelector(p), shuffleButton(), abCompareComponent(p)
{
    // Custom components (Batch 06) - Add BEFORE sliders to ensure on top
    addAndMakeVisible(waveformSelector);
    waveformSelector.setAlwaysOnTop(true);
    addAndMakeVisible(splitToggle);
    splitToggle.setAlwaysOnTop(true);
    addAndMakeVisible(powerButton);
    powerButton.setAlwaysOnTop(true);
    addAndMakeVisible(colorControl);
    colorControl.setAlwaysOnTop(true);
    addAndMakeVisible(midiToggle);
    midiToggle.setAlwaysOnTop(true);
    addAndMakeVisible(retriggerModeSelector);
    retriggerModeSelector.setAlwaysOnTop(true);
    addAndMakeVisible(shuffleButton);
    shuffleButton.setAlwaysOnTop(true);
    addAndMakeVisible(abCompareComponent);
    abCompareComponent.setAlwaysOnTop(true);

    // Shuffle button callback - reset internal state AND clear oscilloscope buffers
    shuffleButton.onShuffle = [this]() {
        audioProcessor.requestShuffle();  // Thread-safe request to audio thread
        envelopeView.clearDisplay();      // Clear UI display immediately
    };

    setupKnob(threshSlider, "THRESHOLD", threshAtt, " dB");
    setupKnob(ceilingSlider,"CEILING",   ceilingAtt," dB");
    setupKnob(relSlider,    "DET_REL",   relAtt,    " ms");
    setupKnob(freqSlider,   "F_FREQ",    freqAtt,   " Hz");
    setupKnob(qSlider,      "F_Q",       qAtt,      "");

    setupKnob(startFreqSlider, "START_FREQ", startFreqAtt, " Hz");
    setupKnob(peakFreqSlider,  "PEAK_FREQ",  peakFreqAtt,  " Hz");
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
    saveButton.setButtonText("");  // Empty text, icon will be drawn in paint()
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    saveButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    saveButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    saveButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));
    saveButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    saveButton.onClick = [this] {
        // Placeholder: save preset logic
        presetNameLabel.setText("Custom Preset", juce::dontSendNotification);
    };

    addAndMakeVisible(loadButton);
    loadButton.setButtonText("");  // Empty text, icon will be drawn in paint()
    loadButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    loadButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    loadButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));
    loadButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    loadButton.onClick = [this] {
        // Create popup menu with 15 presets across 3 categories
        juce::PopupMenu menu;
        menu.setLookAndFeel(&stealthLnF);  // Apply custom industrial theme styling

        menu.addSectionHeader("REALISTIC");  // All-caps for section headers
        menu.addItem(1, "Gunshot");
        menu.addItem(2, "Cannon");
        menu.addItem(3, "Footstep");
        menu.addItem(4, "Door Slam");
        menu.addItem(5, "Thunder");

        menu.addSeparator();
        menu.addSectionHeader("SCI-FI");
        menu.addItem(6, "Laser");
        menu.addItem(7, "Pulse");
        menu.addItem(8, "Energy Shield");
        menu.addItem(9, "Portal");
        menu.addItem(10, "Drone");

        menu.addSeparator();
        menu.addSectionHeader("MUSIC");
        menu.addItem(11, "808 Kick");
        menu.addItem(12, "Sub Drop");
        menu.addItem(13, "Boom Bap");
        menu.addItem(14, "Deep House");
        menu.addItem(15, "Trap 808");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&loadButton),
                           [this](int result) {
                               if (result > 0) {
                                   audioProcessor.loadPreset(result);
                                   // Update preset name
                                   juce::String presetName;
                                   switch(result) {
                                       case 1: presetName = "Gunshot"; break;
                                       case 2: presetName = "Cannon"; break;
                                       case 3: presetName = "Footstep"; break;
                                       case 4: presetName = "Door Slam"; break;
                                       case 5: presetName = "Thunder"; break;
                                       case 6: presetName = "Laser"; break;
                                       case 7: presetName = "Pulse"; break;
                                       case 8: presetName = "Energy Shield"; break;
                                       case 9: presetName = "Portal"; break;
                                       case 10: presetName = "Drone"; break;
                                       case 11: presetName = "808 Kick"; break;
                                       case 12: presetName = "Sub Drop"; break;
                                       case 13: presetName = "Boom Bap"; break;
                                       case 14: presetName = "Deep House"; break;
                                       case 15: presetName = "Trap 808"; break;
                                   }
                                   presetNameLabel.setText(presetName, juce::dontSendNotification);
                               }
                           });
    };

    addAndMakeVisible(envelopeView);
    addAndMakeVisible(energyTopology);

    // MIDI Virtual Keyboard setup (uses processor's keyboardState for external MIDI sync)
    virtualKeyboard.reset(new VirtualKeyboardComponent(audioProcessor.keyboardState));
    addChildComponent(*virtualKeyboard);  // addChildComponent = hidden by default
    virtualKeyboard->setAlwaysOnTop(true);

    // MIDI Toggle callback - show/hide keyboard AND enable MIDI mode
    midiToggle.onMidiModeChanged = [this](bool enabled) {
        showKeyboard = enabled;
        virtualKeyboard->setVisible(showKeyboard);

        // Automatically enable/disable MIDI_MODE parameter when toggling keyboard
        if (auto* midiParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts->getParameter("MIDI_MODE")))
        {
            midiParam->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
        }

        if (showKeyboard)
        {
            setSize(960, 720);  // Expand height for keyboard
        }
        else
        {
            setSize(960, 620);  // Normal height
        }
        resized();
    };

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
    powerButton.setPalette(palette);
    colorControl.setPalette(palette);  // COLOR control theme update
    midiToggle.setPalette(palette);  // MIDI toggle theme update
    retriggerModeSelector.setPalette(palette);  // Retrigger mode selector theme update
    virtualKeyboard->setPalette(palette);  // Virtual keyboard theme update
    shuffleButton.setPalette(palette);  // Shuffle button theme update
    abCompareComponent.setPalette(palette);  // A/B compare theme update

    // Apply colors to sliders (use accent with alpha from map for text box color)
    auto apply = [&](juce::Slider& s) {
        s.setColour(juce::Slider::thumbColourId, palette.accent);
        s.setColour(juce::Slider::rotarySliderFillColourId, palette.accent);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, palette.panel900.brighter(0.2f));

        // Apply text color with current alpha
        float alpha = knobTextAlpha[&s];
        s.setColour(juce::Slider::textBoxTextColourId, palette.accent.withAlpha(alpha));
    };
    apply(threshSlider); apply(ceilingSlider); apply(relSlider); apply(freqSlider); apply(qSlider);
    apply(startFreqSlider); apply(peakFreqSlider); apply(noiseSlider);
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
    // Modern headphone icon (particle style) - 10x10 grid
    // Design: Over-ear headphones with curved headband
    std::vector<std::string> icon = {
        "0011111100",  // Top headband curve
        "0100000010",  // Headband inner
        "1000000001",  // Headband outer arms
        "1000000001",  // Arms extend down
        "1100000011",  // Ear cup top
        "1110000111",  // Ear cup main (thicker)
        "1110000111",  // Ear cup main
        "0110000110",  // Ear cup bottom
        "0000000000",  // Gap
        "0000000000"   // Empty row
    };

    g.setColour(c);
    for(int r=0; r<10; ++r)
        for(int col=0; col<10; ++col)
            if(icon[r][col]=='1')
                g.fillEllipse(x+col*scale, y+r*scale, scale, scale);  // Use circles for particle look
}

void NewProjectAudioProcessorEditor::drawSaveIcon(juce::Graphics& g, int x, int y, int scale, juce::Colour c)
{
    // Floppy disk icon (particle style) - 8x8 grid
    std::vector<std::string> icon = {
        "11111111",  // Top edge
        "10000001",  // Label area
        "10000001",
        "11111111",  // Label bottom
        "10000001",  // Disk body
        "10000001",
        "10011001",  // Metal shutter
        "11111111"   // Bottom edge
    };

    g.setColour(c);
    for(int r=0; r<8; ++r)
        for(int col=0; col<8; ++col)
            if(icon[r][col]=='1')
                g.fillEllipse(x+col*scale, y+r*scale, scale, scale);  // Use circles for particle look
}

void NewProjectAudioProcessorEditor::drawLoadIcon(juce::Graphics& g, int x, int y, int scale, juce::Colour c)
{
    // Folder icon (particle style) - 8x8 grid
    std::vector<std::string> icon = {
        "00000000",
        "01111000",  // Folder tab
        "10000100",
        "10000010",  // Folder body
        "10000001",
        "10000001",
        "10000001",
        "11111111"   // Bottom edge
    };

    g.setColour(c);
    for(int r=0; r<8; ++r)
        for(int col=0; col<8; ++col)
            if(icon[r][col]=='1')
                g.fillEllipse(x+col*scale, y+r*scale, scale, scale);  // Use circles for particle look
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
    updateKnobAlpha(freqSlider); updateKnobAlpha(qSlider);
    updateKnobAlpha(startFreqSlider); updateKnobAlpha(peakFreqSlider); updateKnobAlpha(noiseSlider);
    updateKnobAlpha(pAttSlider); updateKnobAlpha(pDecSlider); updateKnobAlpha(aAttSlider); updateKnobAlpha(aDecSlider);
    updateKnobAlpha(duckSlider); updateKnobAlpha(duckAttSlider); updateKnobAlpha(duckDecSlider);
    updateKnobAlpha(wetSlider); updateKnobAlpha(drySlider); updateKnobAlpha(mixSlider);

    // Update trigger particle scatter effect in Energy Topology
    energyTopology.setTriggerState(audioProcessor.isTriggeredUI);

    // Update bypass and COLOR states to Energy Topology (V19.0)
    bool isBypassed = audioProcessor.apvts->getRawParameterValue("BYPASS")->load() > 0.5f;
    float colorAmount = audioProcessor.apvts->getRawParameterValue("COLOR_AMOUNT")->load();
    energyTopology.setBypassState(isBypassed);
    energyTopology.setSaturation(colorAmount);

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

    // Calculate envelope area (for Scale control and trigger icon)
    const int headerHeight = 26;
    envelopeArea = detectorPanel.withTrimmedTop(headerHeight).reduced(4);

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

    // SAVE and LOAD button icons (particle style, replacing text)
    juce::Colour buttonColor = c_text.withAlpha(0.6f);
    int iconScale = 2;  // 2px per particle

    // SAVE icon (floppy disk)
    int saveIconX = saveButton.getX() + (saveButton.getWidth() - 8*iconScale) / 2;  // Center in button
    int saveIconY = saveButton.getY() + (saveButton.getHeight() - 8*iconScale) / 2;
    drawSaveIcon(g, saveIconX, saveIconY, iconScale, buttonColor);

    // LOAD icon (folder)
    int loadIconX = loadButton.getX() + (loadButton.getWidth() - 8*iconScale) / 2;  // Center in button
    int loadIconY = loadButton.getY() + (loadButton.getHeight() - 8*iconScale) / 2;
    drawLoadIcon(g, loadIconX, loadIconY, iconScale, buttonColor);

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
    drawLabel(freqSlider, "Freq"); drawLabel(qSlider, "Q");
    drawLabel(startFreqSlider, "Start"); drawLabel(peakFreqSlider, "Peak"); drawLabel(noiseSlider, "Noise");
    drawLabel(pAttSlider, "P.Att"); drawLabel(pDecSlider, "P.Dec"); drawLabel(aAttSlider, "A.Att"); drawLabel(aDecSlider, "A.Dec");
    drawLabel(duckSlider, "Duck"); drawLabel(duckAttSlider, "D.Att"); drawLabel(duckDecSlider, "D.Dec");
    drawLabel(wetSlider, "Wet"); drawLabel(drySlider, "Dry"); drawLabel(mixSlider, "Mix");

    // Scale control (below envelope view, centered)
    float scaleValue = audioProcessor.apvts->getRawParameterValue("DET_SCALE")->load();
    juce::String scaleText = juce::String((int)scaleValue) + "%";

    // Calculate scale control area: below envelopeArea, centered horizontally
    int scaleTextWidth = 50;
    int scaleTextHeight = 24;
    scaleControlArea = juce::Rectangle<int>(
        envelopeArea.getX() + (envelopeArea.getWidth() - scaleTextWidth) / 2,  // Centered
        envelopeArea.getBottom() + 5,  // 5px below envelope area
        scaleTextWidth,
        scaleTextHeight
    );

    // Draw scale text with theme color (style reference: images 1-3)
    juce::Colour scaleColor = isDraggingScale ? c_accent : c_accent.withAlpha(0.7f);
    g.setColour(scaleColor);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));  // Larger font like reference images
    g.drawText(scaleText, scaleControlArea, juce::Justification::centred);

    // Footer branding removed (per user request - causes lag even with static white colors)

    // MIDI Digital Display (left of MIDI toggle button)
    int midiNote = audioProcessor.lastMidiNoteUI.load();
    float midiFreq = audioProcessor.lastFrequencyUI.load();

    if (midiNote >= 0)
    {
        // Position: left of MIDI toggle (which is at x=926, y=596)
        // OUTPUT label is at x=700, so position at x=820 to avoid overlap
        // Display width ~100px, leaves safe space before MIDI toggle at x=926
        drawMidiDisplay(g, 820, 598, midiNote, midiFreq, c_accent);
    }
}

void NewProjectAudioProcessorEditor::drawDigitalNumber(juce::Graphics& g, int x, int y, int digit, float scale, juce::Colour baseColor)
{
    // Seven-segment display layout (standard calculator/watch style)
    // Segments numbered:
    //     0
    //   1   2
    //     3
    //   4   5
    //     6

    const float segLength = 8.0f * scale;   // Horizontal segment length
    const float segWidth = 1.5f * scale;    // Segment thickness
    const float segGap = 1.0f * scale;      // Gap between segments

    // Define which segments light up for each digit (0-9)
    const bool segmentMap[10][7] = {
        {true, true, true, false, true, true, true},      // 0
        {false, false, true, false, false, true, false},  // 1
        {true, false, true, true, true, false, true},     // 2
        {true, false, true, true, false, true, true},     // 3
        {false, true, true, true, false, true, false},    // 4
        {true, true, false, true, false, true, true},     // 5
        {true, true, false, true, true, true, true},      // 6
        {true, false, true, false, false, true, false},   // 7
        {true, true, true, true, true, true, true},       // 8
        {true, true, true, true, false, true, false}      // 9 (修复：底部段应该关闭)
    };

    if (digit < 0 || digit > 9) return;

    // Create gradient (darker at bottom, brighter at top)
    juce::ColourGradient gradient(
        baseColor.darker(0.3f), x + segLength * 0.5f, y + segLength * 2.0f + segGap * 3.0f,
        baseColor.brighter(0.4f), x + segLength * 0.5f, y,
        false
    );

    auto drawHorizontalSegment = [&](float sx, float sy, bool active) {
        if (!active) return;

        juce::Path seg;
        seg.startNewSubPath(sx + segWidth, sy);
        seg.lineTo(sx + segLength - segWidth, sy);
        seg.lineTo(sx + segLength, sy + segWidth * 0.5f);
        seg.lineTo(sx + segLength - segWidth, sy + segWidth);
        seg.lineTo(sx + segWidth, sy + segWidth);
        seg.lineTo(sx, sy + segWidth * 0.5f);
        seg.closeSubPath();

        // Glow effect
        juce::DropShadow glow(baseColor.withAlpha(0.8f), 4, juce::Point<int>(0, 0));
        glow.drawForPath(g, seg);

        // Fill with gradient
        g.setGradientFill(gradient);
        g.fillPath(seg);
    };

    auto drawVerticalSegment = [&](float sx, float sy, bool active) {
        if (!active) return;

        juce::Path seg;
        seg.startNewSubPath(sx, sy + segWidth);
        seg.lineTo(sx + segWidth * 0.5f, sy);
        seg.lineTo(sx + segWidth, sy + segWidth);
        seg.lineTo(sx + segWidth, sy + segLength - segWidth);
        seg.lineTo(sx + segWidth * 0.5f, sy + segLength);
        seg.lineTo(sx, sy + segLength - segWidth);
        seg.closeSubPath();

        // Glow effect
        juce::DropShadow glow(baseColor.withAlpha(0.8f), 4, juce::Point<int>(0, 0));
        glow.drawForPath(g, seg);

        // Fill with gradient
        g.setGradientFill(gradient);
        g.fillPath(seg);
    };

    // Draw segments based on digit
    const bool* segments = segmentMap[digit];

    // Segment 0 (top)
    drawHorizontalSegment(x, y, segments[0]);

    // Segment 1 (top-left)
    drawVerticalSegment(x, y + segGap, segments[1]);

    // Segment 2 (top-right)
    drawVerticalSegment(x + segLength - segWidth, y + segGap, segments[2]);

    // Segment 3 (middle)
    drawHorizontalSegment(x, y + segLength + segGap, segments[3]);

    // Segment 4 (bottom-left)
    drawVerticalSegment(x, y + segLength + segGap * 2, segments[4]);

    // Segment 5 (bottom-right)
    drawVerticalSegment(x + segLength - segWidth, y + segLength + segGap * 2, segments[5]);

    // Segment 6 (bottom)
    drawHorizontalSegment(x, y + segLength * 2.0f + segGap * 2.0f, segments[6]);
}

void NewProjectAudioProcessorEditor::drawMidiDisplay(juce::Graphics& g, int x, int y, int midiNote, float frequency, juce::Colour accentColor)
{
    // Convert MIDI note to note name (C, C#, D, etc.) and octave
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int noteIndex = midiNote % 12;
    int octave = (midiNote / 12) - 1;

    juce::String noteName = juce::String(noteNames[noteIndex]) + juce::String(octave);
    juce::String freqText = juce::String((int)frequency) + "Hz";

    // Layout: Note name in text, then frequency in seven-segment digits
    const float scale = 0.9f;  // Reduced scale for compact display
    const float digitSpacing = 9.0f * scale;  // Adjusted spacing (was 7.0f)

    // Draw note name with glow (e.g., "C4")
    g.setColour(accentColor.darker(0.2f));
    g.setFont(juce::FontOptions(13.5f * scale, juce::Font::bold));  // Adjusted font size (was 12.0f)

    // Glow for note name
    juce::DropShadow textGlow(accentColor.withAlpha(0.6f), 3, juce::Point<int>(0, 0));
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(g.getCurrentFont(), noteName, x, y + 12.0f * scale);

    for (int i = 0; i < glyphs.getNumGlyphs(); ++i)
    {
        juce::Path p;
        glyphs.getGlyph(i).createPath(p);
        textGlow.drawForPath(g, p);
    }

    g.setColour(accentColor);
    g.drawText(noteName, x, y, 35, 16, juce::Justification::centredLeft);  // Smaller text area

    // Draw frequency in seven-segment display
    int freqInt = (int)frequency;
    int xPos = x + 38;  // Offset after note name (reduced from 45)

    // Extract individual digits
    juce::String freqDigits = juce::String(freqInt);
    for (int i = 0; i < freqDigits.length(); ++i)
    {
        int digit = freqDigits[i] - '0';
        drawDigitalNumber(g, xPos, y + 2, digit, scale, accentColor);  // Offset Y slightly
        xPos += digitSpacing;
    }

    // Draw "Hz" suffix
    g.setColour(accentColor.withAlpha(0.7f));
    g.setFont(juce::FontOptions(8.0f * scale, juce::Font::plain));  // Smaller Hz label
    g.drawText("Hz", xPos + 2, y + 8, 25, 12, juce::Justification::centredLeft);
}

void NewProjectAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    // Check if clicking on scale control
    if (scaleControlArea.contains(event.getPosition()))
    {
        isDraggingScale = true;
        scaleValueOnMouseDown = audioProcessor.apvts->getRawParameterValue("DET_SCALE")->load();
        mouseYOnScaleDown = event.getMouseDownY();
        repaint();
    }
}

void NewProjectAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingScale)
    {
        // Calculate delta (inverted: up = increase, down = decrease)
        int deltaY = mouseYOnScaleDown - event.y;  // Inverted
        float sensitivity = 0.5f;  // 0.5% per pixel
        float newValue = scaleValueOnMouseDown + (deltaY * sensitivity);

        // Clamp to range [50, 400]
        newValue = juce::jlimit(50.0f, 400.0f, newValue);

        // Update parameter
        if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(audioProcessor.apvts->getParameter("DET_SCALE")))
        {
            float normalised = param->getNormalisableRange().convertTo0to1(newValue);
            param->setValueNotifyingHost(normalised);
        }

        repaint();
    }
}

void NewProjectAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    if (isDraggingScale)
    {
        isDraggingScale = false;
        repaint();
    }
}

void NewProjectAudioProcessorEditor::resized()
{
    int startY = 290; int colW = 220; int knobSize = 60; int gap = 92;  // Reduced knob size, increased spacing

    // Constants for panel layout
    const int margin = 10;
    const int spacing = 10;
    const int topPanelHeight = 230;
    const int headerHeight = 26;
    const int detectorPanelX = 10;
    const int detectorPanelY = 10;
    const int detectorPanelWidth = 470;

    // Calculate panel areas (must match paint())
    juce::Rectangle<int> detectorPanel(margin, margin, 470, topPanelHeight);
    juce::Rectangle<int> topologyPanel(detectorPanel.getRight() + spacing, margin, 460, topPanelHeight);

    // Calculate envelope and topology areas
    envelopeArea = detectorPanel.withTrimmedTop(headerHeight).reduced(4);
    topologyArea = topologyPanel.withTrimmedTop(headerHeight).reduced(4);

    // Set EnvelopeView bounds
    envelopeView.setBounds(envelopeArea);

    // Calculate Scale control area (bottom-right of envelope area)
    int scaleTextWidth = 50;
    int scaleTextHeight = 24;
    scaleControlArea = juce::Rectangle<int>(
        envelopeArea.getRight() - scaleTextWidth - 10,
        envelopeArea.getBottom() - scaleTextHeight - 10,
        scaleTextWidth,
        scaleTextHeight
    );

    // Web-Style Header (top bar) - Compact shuffle button between SAVE and LOAD
    saveButton.setBounds(650, 5, 60, 24);
    shuffleButton.setBounds(715, 5, 30, 24);  // Compact 30px width, between save and load
    loadButton.setBounds(750, 5, 60, 24);
    themeSelector.setBounds(820, 5, 110, 24);

    // A/B Compare buttons - top bar center (between preset name and save button)
    // Layout: [A 20px] [gap 4px] [Pyramid 24px] [gap 4px] [B 20px] = 72px total
    // Position: shifted left to avoid overlapping ENERGY TOPOLOGY text
    abCompareComponent.setBounds(420, 5, 72, 24);

    // Preset Name Label - positioned INSIDE INPUT DETECTOR panel header (green box left)
    // Place in the center-right of INPUT DETECTOR header area
    presetNameLabel.setBounds(detectorPanelX + 180,  // Center-right in detector header
                              detectorPanelY + 6,     // Vertically centered in 26px header
                              200, 14);               // Width 200px, height 14px

    // Power button - positioned at ENERGY TOPOLOGY panel right corner (moved down by button diameter)
    // TOPOLOGY panel: starts at 490, width 460, so right edge is at 950
    int topologyPanelX = detectorPanelX + detectorPanelWidth + spacing;  // 490
    int topologyPanelWidth = 460;
    powerButton.setBounds(topologyPanelX + topologyPanelWidth - 44,  // Right edge minus 44px = 906
                          detectorPanelY + 4 + 36,                   // Y=14+36=50, moved down by button diameter
                          36, 36);                                     // 36x36 button

    // Custom components (Batch 06)
    retriggerModeSelector.setBounds(20 + colW - 110, startY + gap*2, 100, 28);  // Left of waveform selector
    waveformSelector.setBounds(20 + colW, startY + gap*2, 150, 28);  // Replace shapeBox
    splitToggle.setBounds(850, startY, 75, 75);  // Output panel right side, no overlap

    // Energy Topology bounds (matches paint() panel calculation)
    juce::Rectangle<int> topologyPanelArea(490, margin, 460, topPanelHeight);
    energyTopology.setBounds(topologyPanelArea.withTrimmedTop(headerHeight).reduced(4));

    threshSlider.setBounds (20, startY, knobSize, knobSize);
    ceilingSlider.setBounds(100, startY, knobSize, knobSize);
    relSlider.setBounds(20, startY + gap, knobSize, knobSize);
    freqSlider.setBounds(100, startY + gap, knobSize, knobSize);  // Move Freq to where Wait was
    qSlider.setBounds(20, startY + gap*2, knobSize, knobSize);
    auditionButton.setBounds(20, startY + gap*3, 40, 25);
    startFreqSlider.setBounds (20 + colW, startY, knobSize, knobSize);
    peakFreqSlider.setBounds(100 + colW, startY, knobSize, knobSize);

    // COLOR Control - replaces satSlider, larger custom component
    colorControl.setBounds(20 + colW, startY + gap, 150, 90);

    // Noise slider - moved to right of COLOR control
    noiseSlider.setBounds(175 + colW, startY + gap + 15, knobSize, knobSize);
    pAttSlider.setBounds (20 + colW*2, startY, knobSize, knobSize); pDecSlider.setBounds(100 + colW*2, startY, knobSize, knobSize); aAttSlider.setBounds(20 + colW*2, startY + gap, knobSize, knobSize); aDecSlider.setBounds(100 + colW*2, startY + gap, knobSize, knobSize);
    duckSlider.setBounds (20 + colW*3, startY, knobSize, knobSize); mixSlider.setBounds(100 + colW*3, startY, knobSize, knobSize); duckAttSlider.setBounds(20 + colW*3, startY + gap, knobSize, knobSize); duckDecSlider.setBounds(100 + colW*3, startY + gap, knobSize, knobSize); wetSlider.setBounds(20 + colW*3, startY + gap*2, knobSize, knobSize); drySlider.setBounds(100 + colW*3, startY + gap*2, knobSize, knobSize);

    // MIDI Toggle - small icon in bottom-right corner (always at same position)
    // Position relative to 620px height, stays fixed even when keyboard expands window
    midiToggle.setBounds(getWidth() - 34, 620 - 24, 24, 16);

    // Virtual Keyboard (bottom of window when visible)
    if (showKeyboard)
    {
        int keyboardHeight = 100;
        virtualKeyboard->setBounds(0, getHeight() - keyboardHeight, getWidth(), keyboardHeight);
    }
}
