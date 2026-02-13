#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CarTestAudioProcessor::CarTestAudioProcessor()
    : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    presetParam      = apvts.getRawParameterValue ("preset");
    noiseAmountParam = apvts.getRawParameterValue ("noiseAmount");
}

CarTestAudioProcessor::~CarTestAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
CarTestAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Preset selector:  0=Bypass, 1=Sedan, 2=Phone, 3=Laptop, 4=BT Speaker
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "preset", 1 }, "Environment", 0, 4, 0));

    // Background noise amount  0..1
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noiseAmount", 1 }, "Noise",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
void CarTestAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    envProcessor.prepare (spec);
    noiseGen.prepare (sampleRate, samplesPerBlock);
}

void CarTestAudioProcessor::releaseResources()
{
    envProcessor.reset();
    noiseGen.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CarTestAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Support mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

//==============================================================================
void CarTestAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear extra output channels
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Read parameters
    const int   presetIdx  = static_cast<int> (presetParam->load());
    const float noiseAmt   = noiseAmountParam->load();

    // If bypass and no noise, early out
    if (presetIdx == 0 && noiseAmt < 0.0001f)
        return;

    // Apply environment processing
    envProcessor.setPreset (presetIdx);
    envProcessor.process (buffer);

    // Add background noise
    noiseGen.process (buffer, noiseAmt);
}

//==============================================================================
bool CarTestAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* CarTestAudioProcessor::createEditor()
{
    return new CarTestAudioProcessorEditor (*this);
}

//==============================================================================
const juce::String CarTestAudioProcessor::getName() const { return JucePlugin_Name; }
bool   CarTestAudioProcessor::acceptsMidi()  const { return false; }
bool   CarTestAudioProcessor::producesMidi() const { return false; }
bool   CarTestAudioProcessor::isMidiEffect() const { return false; }
double CarTestAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int    CarTestAudioProcessor::getNumPrograms()    { return 1; }
int    CarTestAudioProcessor::getCurrentProgram() { return 0; }
void   CarTestAudioProcessor::setCurrentProgram (int) {}
const juce::String CarTestAudioProcessor::getProgramName (int) { return {}; }
void   CarTestAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void CarTestAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CarTestAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::StringArray CarTestAudioProcessor::getPresetNames() const
{
    return { "Bypass", "The Sedan", "The Phone", "The Laptop", "The BT Speaker" };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CarTestAudioProcessor();
}
