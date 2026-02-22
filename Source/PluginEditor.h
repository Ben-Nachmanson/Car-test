#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Automotive colour palette (shared by editor + LookAndFeel classes)
namespace DashColours
{
    static inline const juce::Colour buttonOff      { 0xaa1a1a1a };   // dark glass
    static inline const juce::Colour buttonOn       { 0xff2a2218 };   // recessed dark
    static inline const juce::Colour amberLED       { 0xffffaa22 };   // amber indicator
    static inline const juce::Colour textBright     { 0xffeee8dd };   // warm white
    static inline const juce::Colour textDim        { 0xff998877 };   // muted tan
    static inline const juce::Colour chrome         { 0xffc0bab0 };   // brushed aluminum
    static inline const juce::Colour chromeDark     { 0xff706860 };   // dark chrome rim
    static inline const juce::Colour knobFill       { 0xff2a2520 };   // knob centre
    static inline const juce::Colour knobTrack      { 0xff554840 };   // knob arc bg
    static inline const juce::Colour knobArc        { 0xffffaa22 };   // knob arc active (amber)
}

//==============================================================================
/**
    Custom LookAndFeel for dashboard-style buttons:
      - OFF: dark glass overlay, subtle border
      - ON:  recessed look with amber LED indicator at top edge
*/
class DashboardLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DashboardLookAndFeel();

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    // Disable focus outline — LED dot is the only selection indicator
    std::unique_ptr<juce::FocusOutline> createFocusOutlineForComponent (juce::Component&) override
    {
        return nullptr;
    }
};

//==============================================================================
/**
    Custom LookAndFeel for the chrome-rimmed rotary noise knob.
*/
class ChromeKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChromeKnobLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

//==============================================================================
class CarTestAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit CarTestAudioProcessorEditor (CarTestAudioProcessor&);
    ~CarTestAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    CarTestAudioProcessor& processorRef;

    // Dashboard background image
    juce::Image dashboardBg;

    // Environment preset buttons
    juce::TextButton bypassButton   { "BYPASS" };
    juce::TextButton sedanButton    { "CAR" };
    juce::TextButton phoneButton    { "PHONE" };
    juce::TextButton laptopButton   { "LAPTOP" };
    juce::TextButton btButton       { "BT SPEAKER" };

    // Noise knob
    juce::Slider noiseSlider;
    juce::Label  noiseLabel { {}, "CITY NOISE" };

    // APVTS attachment
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseAttachment;

    // Current selected preset (for button highlighting)
    int currentPreset = 0;

    // Custom look and feels
    DashboardLookAndFeel dashboardLnF;
    ChromeKnobLookAndFeel chromeKnobLnF;

    void selectPreset (int index);
    void updateButtonStates();

    std::vector<juce::TextButton*> presetButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CarTestAudioProcessorEditor)
};
