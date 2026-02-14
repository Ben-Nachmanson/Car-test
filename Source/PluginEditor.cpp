#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
//  DashboardLookAndFeel — physical car-button appearance
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
    const float cs = 3.5f;  // tight corner radius — physical car buttons

    const bool isOn = button.getToggleState();

    // ---- Matte button surface with subtle gradient for 3D depth ----
    if (isOn)
    {
        // Active: neutral dark surface, same family as off-state but pressed in
        juce::ColourGradient bgGrad (juce::Colour (0xff222222), bounds.getX(), bounds.getY(),
                                      juce::Colour (0xff181818), bounds.getX(), bounds.getBottom(),
                                      false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (bounds, cs);

        // Recessed inner shadow at top (pressed-in feel)
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (bounds.withHeight (2.5f), cs);
    }
    else
    {
        // Inactive: raised matte surface
        juce::ColourGradient bgGrad (juce::Colour (0xff2a2a2a), bounds.getX(), bounds.getY(),
                                      juce::Colour (0xff1a1a1a), bounds.getX(), bounds.getBottom(),
                                      false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (bounds, cs);

        // Top-edge highlight (light catching the raised surface)
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (bounds.withHeight (1.5f), cs);

        if (isHighlighted || isDown)
        {
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.fillRoundedRectangle (bounds, cs);
        }

        if (isDown)
        {
            // Pressed: invert the emboss
            g.setColour (juce::Colours::black.withAlpha (0.15f));
            g.fillRoundedRectangle (bounds, cs);
        }
    }

    // ---- Crisp thin border ----
    g.setColour (juce::Colours::white.withAlpha (isOn ? 0.12f : 0.06f));
    g.drawRoundedRectangle (bounds, cs, 0.75f);

    // ---- Bottom-edge shadow (depth against panel) ----
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    auto shadowLine = bounds;
    shadowLine = shadowLine.withTop (shadowLine.getBottom() - 1.0f).translated (0.0f, 1.5f);
    g.fillRoundedRectangle (shadowLine, cs);

    // ---- Small LED dot indicator (active only) ----
    if (isOn)
    {
        float dotSize = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.12f;
        dotSize = juce::jlimit (4.0f, 7.0f, dotSize);
        float dotX = bounds.getCentreX() - dotSize * 0.5f;
        float dotY = bounds.getY() + 5.0f;

        // LED glow
        g.setColour (DashColours::amberLED.withAlpha (0.25f));
        g.fillEllipse (dotX - 2.0f, dotY - 2.0f, dotSize + 4.0f, dotSize + 4.0f);

        // LED dot
        g.setColour (DashColours::amberLED);
        g.fillEllipse (dotX, dotY, dotSize, dotSize);
    }
}

void DashboardLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                            bool /*isHighlighted*/, bool /*isDown*/)
{
    auto bounds = button.getLocalBounds();
    const bool isOn = button.getToggleState();

    // Offset text down slightly to account for LED dot space
    if (isOn)
        bounds.translate (0, 2);

    g.setColour (isOn ? DashColours::textBright
                      : DashColours::textDim.withAlpha (0.7f));

    // Scale font with button height for consistency across sizes
    float fontSize = juce::jlimit (9.0f, 12.0f, static_cast<float> (bounds.getHeight()) * 0.32f);
    g.setFont (juce::FontOptions (fontSize, juce::Font::bold));
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
//  Helper: draw a recessed dashboard panel surround
//==============================================================================
static void drawDashPanel (juce::Graphics& g, juce::Rectangle<float> bounds, float cs = 4.0f)
{
    // Subtle recessed panel — just enough to ground the buttons
    g.setColour (juce::Colour (0x60101010));
    g.fillRoundedRectangle (bounds, cs);

    // Very faint top inner shadow
    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.fillRoundedRectangle (bounds.withHeight (1.5f), cs);

    // Thin border
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    g.drawRoundedRectangle (bounds, cs, 0.5f);
}

//==============================================================================
//  CarTestAudioProcessorEditor
//==============================================================================
CarTestAudioProcessorEditor::CarTestAudioProcessorEditor (CarTestAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Load dashboard background from binary data
    dashboardBg = juce::ImageCache::getFromMemory (BinaryData::Dashboard_png, BinaryData::Dashboard_pngSize);

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

    // Lighter overlay — let more of the dashboard photo show through
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRect (area);

    // Top vignette — darken behind the title for legibility
    {
        juce::ColourGradient vignette (juce::Colours::black.withAlpha (0.55f), 0.0f, 0.0f,
                                        juce::Colours::transparentBlack, 0.0f,
                                        static_cast<float> (area.getHeight()) * 0.22f, false);
        g.setGradientFill (vignette);
        g.fillRect (area.removeFromTop (static_cast<int> (area.getHeight() * 0.22f)));
    }

    const float sx = static_cast<float> (getWidth())  / 650.0f;
    const float sy = static_cast<float> (getHeight()) / 380.0f;
    const float s  = juce::jmin (sx, sy);

    // ---- Recessed panel behind CAR button (head unit area) ----
    {
        float panelX = 258.0f * sx;
        float panelY = 136.0f * sy;
        float panelW = 134.0f * sx;
        float panelH = 52.0f  * sy;
        drawDashPanel (g, { panelX, panelY, panelW, panelH }, 5.0f * s);
    }

    // ---- Recessed panel behind passenger-side buttons ----
    {
        float panelX = 434.0f * sx;
        float panelY = 100.0f * sy;
        float panelW = 206.0f * sx;
        float panelH = 52.0f  * sy;
        drawDashPanel (g, { panelX, panelY, panelW, panelH }, 5.0f * s);
    }

    // ---- Recessed panel behind BYPASS button ----
    {
        float panelX = 16.0f  * sx;
        float panelY = 318.0f * sy;
        float panelW = 96.0f  * sx;
        float panelH = 46.0f  * sy;
        drawDashPanel (g, { panelX, panelY, panelW, panelH }, 5.0f * s);
    }

    // ---- "CAR TEST" branding — top centre (multi-pass for depth) ----
    {
        auto titleArea = getLocalBounds().removeFromTop (static_cast<int> (44.0f * sy));
        g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));

        // Pass 1: soft dark shadow for separation
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText ("CAR TEST", titleArea.translated (0, static_cast<int> (2.0f * s)),
                     juce::Justification::centred);

        // Pass 2: subtle amber glow (slightly larger offset spread)
        g.setColour (DashColours::amberLED.withAlpha (0.18f));
        g.drawText ("CAR TEST", titleArea.translated (static_cast<int> (-1.0f * s), 0),
                     juce::Justification::centred);
        g.drawText ("CAR TEST", titleArea.translated (static_cast<int> (1.0f * s), 0),
                     juce::Justification::centred);
        g.drawText ("CAR TEST", titleArea.translated (0, static_cast<int> (-1.0f * s)),
                     juce::Justification::centred);
        g.drawText ("CAR TEST", titleArea.translated (0, static_cast<int> (1.0f * s)),
                     juce::Justification::centred);

        // Pass 3: crisp main title at full opacity
        g.setColour (DashColours::amberLED);
        g.drawText ("CAR TEST", titleArea, juce::Justification::centred);
    }

    // ---- Subtitle ----
    {
        g.setColour (DashColours::textDim.withAlpha (0.6f));
        g.setFont (juce::FontOptions (9.0f * s));
        g.drawText ("Save yourself the trip to the driveway.",
                     getLocalBounds().withHeight (static_cast<int> (56.0f * sy))
                                     .translated (0, static_cast<int> (26.0f * sy)),
                     juce::Justification::centred);
    }
}

void CarTestAudioProcessorEditor::resized()
{
    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());
    const float sx = w / 650.0f;
    const float sy = h / 380.0f;

    auto scaled = [&] (float x, float y, float bw, float bh) -> juce::Rectangle<int>
    {
        return { static_cast<int> (x * sx), static_cast<int> (y * sy),
                 static_cast<int> (bw * sx), static_cast<int> (bh * sy) };
    };

    // ---- CAR button: centre console, head unit area (radio button) ----
    sedanButton.setBounds (scaled (265.0f, 142.0f, 120.0f, 40.0f));

    // ---- BYPASS: bottom-left, near steering column ----
    bypassButton.setBounds (scaled (22.0f, 324.0f, 84.0f, 34.0f));

    // ---- PHONE / LAPTOP / BT SPEAKER: passenger side dash, horizontal row ----
    const float passBtnW = 60.0f;
    const float passBtnH = 36.0f;
    const float passGap  = 6.0f;
    const float passStartX = 440.0f;
    const float passY = 106.0f;

    phoneButton.setBounds  (scaled (passStartX,                              passY, passBtnW, passBtnH));
    laptopButton.setBounds (scaled (passStartX + 1.0f * (passBtnW + passGap), passY, passBtnW, passBtnH));
    btButton.setBounds     (scaled (passStartX + 2.0f * (passBtnW + passGap), passY, passBtnW + 10.0f, passBtnH));

    // ---- Noise knob: lower-right, HVAC area ----
    int knobSize = static_cast<int> (56.0f * sx);
    int knobX    = static_cast<int> (520.0f * sx);
    int knobY    = static_cast<int> (285.0f * sy);
    noiseSlider.setBounds (knobX, knobY, knobSize, knobSize);
    noiseLabel.setBounds  (knobX - static_cast<int> (8.0f * sx), knobY + knobSize + 2,
                           knobSize + static_cast<int> (16.0f * sx),
                           static_cast<int> (14.0f * sy));
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
