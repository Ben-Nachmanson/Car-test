#pragma once

#include <JuceHeader.h>
#include "DSP/EnvironmentProcessor.h"
#include "DSP/NoiseGenerator.h"

//==============================================================================
class CarTestAudioProcessor : public juce::AudioProcessor
{
public:
    CarTestAudioProcessor();
    ~CarTestAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==========================================================================
    const juce::String getName() const override;

    bool   acceptsMidi()  const override;
    bool   producesMidi() const override;
    bool   isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==========================================================================
    int  getNumPrograms() override;
    int  getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Convenience: get the list of preset names for the UI
    juce::StringArray getPresetNames() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    EnvironmentProcessor envProcessor;
    NoiseGenerator       noiseGen;

    // Atomic parameter caches (read in processBlock)
    std::atomic<float>* presetParam     = nullptr;
    std::atomic<float>* noiseAmountParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CarTestAudioProcessor)
};
