#include "PluginEditor.h"

//==============================================================================
CarTestAudioProcessorEditor::CarTestAudioProcessorEditor (CarTestAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Collect preset buttons into a vector for easy iteration
    presetButtons = { &bypassButton, &sedanButton, &phoneButton, &laptopButton, &btButton };

    for (size_t i = 0; i < presetButtons.size(); ++i)
    {
        auto* btn = presetButtons[i];
        addAndMakeVisible (btn);
        btn->setClickingTogglesState (false);
        btn->onClick = [this, idx = static_cast<int> (i)] { selectPreset (idx); };
    }

    // --- Noise slider ---
    addAndMakeVisible (noiseSlider);
    noiseSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    noiseSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    noiseSlider.setColour (juce::Slider::rotarySliderFillColourId, Colours::sliderThumb);
    noiseSlider.setColour (juce::Slider::rotarySliderOutlineColourId, Colours::sliderTrack);
    noiseSlider.setColour (juce::Slider::thumbColourId, Colours::sliderThumb);
    noiseSlider.setColour (juce::Slider::textBoxTextColourId, Colours::textBright);
    noiseSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    addAndMakeVisible (noiseLabel);
    noiseLabel.setJustificationType (juce::Justification::centred);
    noiseLabel.setColour (juce::Label::textColourId, Colours::textDim);
    noiseLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));

    // --- Mix slider ---
    addAndMakeVisible (mixSlider);
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    mixSlider.setColour (juce::Slider::rotarySliderFillColourId, Colours::accent);
    mixSlider.setColour (juce::Slider::rotarySliderOutlineColourId, Colours::sliderTrack);
    mixSlider.setColour (juce::Slider::thumbColourId, Colours::accent);
    mixSlider.setColour (juce::Slider::textBoxTextColourId, Colours::textBright);
    mixSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    addAndMakeVisible (mixLabel);
    mixLabel.setJustificationType (juce::Justification::centred);
    mixLabel.setColour (juce::Label::textColourId, Colours::textDim);
    mixLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));

    // --- Windows Down toggle ---
    addAndMakeVisible (windowsDownToggle);
    windowsDownToggle.setColour (juce::ToggleButton::textColourId, Colours::textBright);
    windowsDownToggle.setColour (juce::ToggleButton::tickColourId, Colours::windowsDown);

    // --- APVTS Attachments ---
    noiseAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                               processorRef.getAPVTS(), "noiseAmount", noiseSlider);
    mixAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                               processorRef.getAPVTS(), "mix", mixSlider);
    windowsDownAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                               processorRef.getAPVTS(), "windowsDown", windowsDownToggle);

    // Timer to keep button highlighting in sync with automation
    startTimerHz (15);

    updateButtonStates();
    setSize (480, 360);
}

CarTestAudioProcessorEditor::~CarTestAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void CarTestAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background gradient
    g.fillAll (Colours::background);

    // Title bar area
    auto titleArea = getLocalBounds().removeFromTop (50);
    g.setColour (Colours::panelDark);
    g.fillRect (titleArea);

    g.setColour (Colours::textBright);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("CAR TEST", titleArea, juce::Justification::centred);

    // Subtitle
    g.setColour (Colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("Save yourself the trip to the driveway.",
                titleArea.translated (0, 18), juce::Justification::centred);

    // Separator line
    g.setColour (Colours::highlight);
    g.fillRect (0, 50, getWidth(), 2);

    // Panel for knobs area
    auto knobArea = getLocalBounds().reduced (10).removeFromBottom (140);
    g.setColour (Colours::panelDark.withAlpha (0.5f));
    g.fillRoundedRectangle (knobArea.toFloat(), 8.0f);
}

void CarTestAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Title bar
    area.removeFromTop (56);

    // Preset buttons row
    auto buttonRow = area.removeFromTop (55).reduced (10, 8);
    const int buttonWidth = buttonRow.getWidth() / static_cast<int> (presetButtons.size()) - 6;

    for (auto* btn : presetButtons)
    {
        btn->setBounds (buttonRow.removeFromLeft (buttonWidth).reduced (2));
        buttonRow.removeFromLeft (4); // spacing
    }

    // Small gap
    area.removeFromTop (5);

    // Windows Down toggle
    auto toggleRow = area.removeFromTop (30).reduced (10, 0);
    windowsDownToggle.setBounds (toggleRow);

    // Knobs row
    area.removeFromTop (5);
    auto knobRow = area.reduced (20, 5);

    const int knobSize = 100;
    const int labelH   = 20;

    // Noise knob (left)
    auto noiseArea = knobRow.removeFromLeft (knobRow.getWidth() / 2);
    noiseLabel.setBounds (noiseArea.removeFromTop (labelH));
    noiseSlider.setBounds (noiseArea.withSizeKeepingCentre (knobSize, knobSize));

    // Mix knob (right)
    auto mixArea = knobRow;
    mixLabel.setBounds (mixArea.removeFromTop (labelH));
    mixSlider.setBounds (mixArea.withSizeKeepingCentre (knobSize, knobSize));
}

//==============================================================================
void CarTestAudioProcessorEditor::timerCallback()
{
    // Sync button highlight with parameter (supports DAW automation)
    auto* param = processorRef.getAPVTS().getRawParameterValue ("preset");
    int idx = static_cast<int> (param->load());

    if (idx != currentPreset)
    {
        currentPreset = idx;
        updateButtonStates();
    }
}

void CarTestAudioProcessorEditor::selectPreset (int index)
{
    processorRef.getAPVTS().getParameterAsValue ("preset").setValue (index);
    currentPreset = index;
    updateButtonStates();
}

void CarTestAudioProcessorEditor::updateButtonStates()
{
    for (size_t i = 0; i < presetButtons.size(); ++i)
    {
        bool selected = (static_cast<int> (i) == currentPreset);
        auto* btn = presetButtons[i];

        btn->setColour (juce::TextButton::buttonColourId,
                        selected ? Colours::buttonOn : Colours::buttonOff);
        btn->setColour (juce::TextButton::textColourOffId, Colours::textBright);
        btn->setColour (juce::TextButton::textColourOnId,  Colours::textBright);
    }

    repaint();
}
