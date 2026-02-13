#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
//  DashboardLookAndFeel
//==============================================================================
DashboardLookAndFeel::DashboardLookAndFeel()
{
    setColour (juce::TextButton::textColourOffId, DashColours::textBright);
    setColour (juce::TextButton::textColourOnId,  DashColours::textBright);
}

void DashboardLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& /*backgroundColour*/,
                                                  bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const float cornerSize = 6.0f;

    const bool isOn = button.getToggleState();

    if (isOn)
    {
        // Recessed: darker fill, inset shadow effect
        g.setColour (DashColours::buttonOn);
        g.fillRoundedRectangle (bounds, cornerSize);

        // Inner shadow (top edge darker)
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (bounds.removeFromTop (3.0f), cornerSize);

        // Amber LED indicator bar at top
        auto ledBounds = button.getLocalBounds().toFloat().reduced (1.0f);
        auto ledBar = ledBounds.removeFromTop (3.0f);
        g.setColour (DashColours::amberLED);
        g.fillRoundedRectangle (ledBar.reduced (8.0f, 0.0f), 1.5f);

        // Subtle amber glow
        g.setColour (DashColours::amberLED.withAlpha (0.08f));
        g.fillRoundedRectangle (button.getLocalBounds().toFloat().reduced (1.0f), cornerSize);
    }
    else
    {
        // Dark glass overlay
        g.setColour (DashColours::buttonOff);
        g.fillRoundedRectangle (bounds, cornerSize);

        if (isHighlighted || isDown)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillRoundedRectangle (bounds, cornerSize);
        }
    }

    // Subtle border
    g.setColour (juce::Colours::white.withAlpha (isOn ? 0.15f : 0.08f));
    g.drawRoundedRectangle (button.getLocalBounds().toFloat().reduced (1.0f), cornerSize, 1.0f);
}

void DashboardLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                            bool /*isHighlighted*/, bool /*isDown*/)
{
    auto bounds = button.getLocalBounds();
    const bool isOn = button.getToggleState();

    g.setColour (isOn ? DashColours::textBright
                      : DashColours::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (button.getButtonText(), bounds, juce::Justification::centred);
}

//==============================================================================
//  ChromeKnobLookAndFeel
//==============================================================================
ChromeKnobLookAndFeel::ChromeKnobLookAndFeel() {}

void ChromeKnobLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                                int x, int y, int width, int height,
                                                float sliderPos,
                                                float rotaryStartAngle,
                                                float rotaryEndAngle,
                                                juce::Slider& /*slider*/)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                           static_cast<float> (width), static_cast<float> (height));
    auto centre = bounds.getCentre();
    float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // Chrome outer rim
    {
        float rimRadius = radius;
        juce::ColourGradient rimGrad (DashColours::chrome, centre.x, centre.y - rimRadius,
                                       DashColours::chromeDark, centre.x, centre.y + rimRadius,
                                       false);
        g.setGradientFill (rimGrad);
        g.fillEllipse (centre.x - rimRadius, centre.y - rimRadius, rimRadius * 2.0f, rimRadius * 2.0f);
    }

    // Knob face (inner circle)
    {
        float innerRadius = radius * 0.82f;
        g.setColour (DashColours::knobFill);
        g.fillEllipse (centre.x - innerRadius, centre.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

        // Subtle inner highlight
        juce::ColourGradient innerGrad (juce::Colours::white.withAlpha (0.08f), centre.x, centre.y - innerRadius,
                                         juce::Colours::transparentBlack, centre.x, centre.y + innerRadius * 0.5f,
                                         false);
        g.setGradientFill (innerGrad);
        g.fillEllipse (centre.x - innerRadius, centre.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
    }

    // Arc track (background)
    {
        float arcRadius = radius * 0.68f;
        juce::Path bgArc;
        bgArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (DashColours::knobTrack);
        g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    // Arc track (active – amber)
    {
        float arcRadius = radius * 0.68f;
        float endAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        if (endAngle > rotaryStartAngle + 0.01f)
        {
            juce::Path activeArc;
            activeArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                      rotaryStartAngle, endAngle, true);
            g.setColour (DashColours::knobArc);
            g.strokePath (activeArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                             juce::PathStrokeType::rounded));
        }
    }

    // Pointer / indicator line
    {
        float pointerRadius = radius * 0.60f;
        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        float pointerLength = radius * 0.30f;

        juce::Path pointer;
        float startR = pointerRadius - pointerLength;
        pointer.startNewSubPath (centre.x + startR * std::cos (angle - juce::MathConstants<float>::halfPi),
                                  centre.y + startR * std::sin (angle - juce::MathConstants<float>::halfPi));
        pointer.lineTo (centre.x + pointerRadius * std::cos (angle - juce::MathConstants<float>::halfPi),
                         centre.y + pointerRadius * std::sin (angle - juce::MathConstants<float>::halfPi));

        g.setColour (DashColours::amberLED);
        g.strokePath (pointer, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }
}

//==============================================================================
//  CarTestAudioProcessorEditor
//==============================================================================
CarTestAudioProcessorEditor::CarTestAudioProcessorEditor (CarTestAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Load dashboard background from binary data
    dashboardBg = juce::ImageCache::getFromMemory (BinaryData::bmw_jpg, BinaryData::bmw_jpgSize);

    // Collect preset buttons
    presetButtons = { &bypassButton, &sedanButton, &phoneButton, &laptopButton, &btButton };

    for (size_t i = 0; i < presetButtons.size(); ++i)
    {
        auto* btn = presetButtons[i];
        addAndMakeVisible (btn);
        btn->setClickingTogglesState (false);
        btn->setLookAndFeel (&dashboardLnF);
        btn->onClick = [this, idx = static_cast<int> (i)] { selectPreset (idx); };
    }

    // --- Noise knob ---
    addAndMakeVisible (noiseSlider);
    noiseSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    noiseSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    noiseSlider.setLookAndFeel (&chromeKnobLnF);

    addAndMakeVisible (noiseLabel);
    noiseLabel.setJustificationType (juce::Justification::centred);
    noiseLabel.setColour (juce::Label::textColourId, DashColours::textBright);
    noiseLabel.setFont (juce::FontOptions (10.0f, juce::Font::bold));

    // --- APVTS Attachment ---
    noiseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                          processorRef.getAPVTS(), "noiseAmount", noiseSlider);

    // Timer to keep button highlighting in sync with automation
    startTimerHz (15);

    updateButtonStates();

    // Resizable from bottom-right corner, with aspect-ratio constraint
    setResizable (true, true);
    setResizeLimits (480, 280, 1300, 760);
    getConstrainer()->setFixedAspectRatio (650.0 / 380.0);
    setSize (650, 380);
}

CarTestAudioProcessorEditor::~CarTestAudioProcessorEditor()
{
    stopTimer();

    // Clear look-and-feel references before destruction
    for (auto* btn : presetButtons)
        btn->setLookAndFeel (nullptr);
    noiseSlider.setLookAndFeel (nullptr);
}

//==============================================================================
void CarTestAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();

    // Draw dashboard background image, scaled to fill
    if (dashboardBg.isValid())
    {
        g.drawImage (dashboardBg, area.toFloat(),
                     juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
    }
    else
    {
        g.fillAll (juce::Colour (0xff1a1510));
    }

    // Semi-transparent dark overlay for readability
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.fillRect (area);

    const float sx = static_cast<float> (getWidth())  / 650.0f;
    const float sy = static_cast<float> (getHeight()) / 380.0f;

    // Paint over the BMW steering wheel logo area
    {
        float cx = 148.0f * sx;
        float cy = 270.0f * sy;
        float r  = 22.0f * juce::jmin (sx, sy);
        g.setColour (juce::Colour (0xff1a1815));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // "CAR TEST" branding - top centre
    {
        g.setColour (DashColours::amberLED.withAlpha (0.9f));
        g.setFont (juce::FontOptions (26.0f * juce::jmin (sx, sy), juce::Font::bold));
        g.drawText ("CAR TEST", getLocalBounds().removeFromTop (static_cast<int> (48.0f * sy)),
                     juce::Justification::centred);
    }

    // Subtitle
    {
        g.setColour (DashColours::textDim);
        g.setFont (juce::FontOptions (10.0f * juce::jmin (sx, sy)));
        g.drawText ("Save yourself the trip to the driveway.",
                     getLocalBounds().withHeight (static_cast<int> (60.0f * sy))
                                     .translated (0, static_cast<int> (28.0f * sy)),
                     juce::Justification::centred);
    }
}

void CarTestAudioProcessorEditor::resized()
{
    // All positions are proportional to the current size so the layout
    // scales when the user resizes from the bottom-right corner.

    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());
    const float sx = w / 650.0f;   // scale factors relative to reference size
    const float sy = h / 380.0f;

    auto scaled = [&] (float x, float y, float bw, float bh) -> juce::Rectangle<int>
    {
        return { static_cast<int> (x * sx), static_cast<int> (y * sy),
                 static_cast<int> (bw * sx), static_cast<int> (bh * sy) };
    };

    // --- CAR button: large, centre of the console ---
    sedanButton.setBounds (scaled (265.0f, 148.0f, 120.0f, 44.0f));

    // --- BYPASS button: bottom-left ---
    bypassButton.setBounds (scaled (24.0f, 326.0f, 80.0f, 34.0f));

    // --- Right side stack: PHONE, LAPTOP, BT SPEAKER ---
    phoneButton.setBounds  (scaled (526.0f, 120.0f, 100.0f, 32.0f));
    laptopButton.setBounds (scaled (526.0f, 160.0f, 100.0f, 32.0f));
    btButton.setBounds     (scaled (526.0f, 200.0f, 100.0f, 32.0f));

    // --- Noise knob: lower-right, over HVAC area ---
    int knobSize = static_cast<int> (64.0f * sx);
    int knobX    = static_cast<int> (514.0f * sx);
    int knobY    = static_cast<int> (280.0f * sy);
    noiseSlider.setBounds (knobX, knobY, knobSize, knobSize);
    noiseLabel.setBounds  (knobX - static_cast<int> (10.0f * sx), knobY + knobSize + 2,
                           knobSize + static_cast<int> (20.0f * sx),
                           static_cast<int> (16.0f * sy));
}

//==============================================================================
void CarTestAudioProcessorEditor::timerCallback()
{
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
        presetButtons[i]->setToggleState (selected, juce::dontSendNotification);
    }

    repaint();
}
