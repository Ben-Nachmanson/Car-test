#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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

    // Environment preset buttons
    juce::TextButton bypassButton   { "BYPASS" };
    juce::TextButton sedanButton    { "THE SEDAN" };
    juce::TextButton phoneButton    { "THE PHONE" };
    juce::TextButton laptopButton   { "THE LAPTOP" };
    juce::TextButton btButton       { "BT SPEAKER" };

    // Noise slider
    juce::Slider noiseSlider;
    juce::Label  noiseLabel { {}, "ROAD NOISE" };

    // Mix slider
    juce::Slider mixSlider;
    juce::Label  mixLabel { {}, "MIX" };

    // Windows Down toggle
    juce::ToggleButton windowsDownToggle { "WINDOWS DOWN" };

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  noiseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>  windowsDownAttachment;

    // Current selected preset (for button highlighting)
    int currentPreset = 0;

    // Custom colours
    struct Colours
    {
        static inline const juce::Colour background   { 0xff1a1a2e };
        static inline const juce::Colour panelDark     { 0xff16213e };
        static inline const juce::Colour accent        { 0xff0f3460 };
        static inline const juce::Colour highlight     { 0xffe94560 };
        static inline const juce::Colour textBright    { 0xffeaeaea };
        static inline const juce::Colour textDim       { 0xff8a8a9a };
        static inline const juce::Colour buttonOff     { 0xff2a2a4a };
        static inline const juce::Colour buttonOn      { 0xffe94560 };
        static inline const juce::Colour sliderTrack   { 0xff3a3a5a };
        static inline const juce::Colour sliderThumb   { 0xffe94560 };
        static inline const juce::Colour windowsDown   { 0xff4ecca3 };
    };

    void selectPreset (int index);
    void updateButtonStates();

    std::vector<juce::TextButton*> presetButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CarTestAudioProcessorEditor)
};
