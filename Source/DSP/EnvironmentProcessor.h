#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <BinaryData.h>

//==============================================================================
/**
    Defines the EQ / processing profile for a single listening environment.
    Each profile includes IIR filter specs, convolution IR (wet/dry blend),
    stereo width, early reflections, and optional compression.
*/
struct EnvironmentPreset
{
    const char* name;

    // High-pass frequency (Hz) – removes bass
    float highPassFreq   = 20.0f;
    // Low-pass frequency (Hz) – removes treble
    float lowPassFreq    = 20000.0f;
    // Peak / resonance EQ bands  (freq, gain dB, Q)
    struct Band { float freq; float gainDb; float q; };
    std::vector<Band> bands;

    // Output gain trim (dB)
    float outputGainDb   = 0.0f;

    // Convolution IR resource (nullptr = no convolution)
    const char* irResourceName = nullptr;
    int         irResourceSize = 0;
    // Wet/dry blend for convolution (0.0 = fully dry, 1.0 = fully wet)
    float irWetMix        = 0.0f;

    // Stereo width via mid-side (0.0 = mono, 1.0 = full stereo)
    float stereoWidth     = 1.0f;

    // Early reflections (car cabin simulation)
    bool earlyReflections = false;

    // Whether to apply simple dynamics compression
    bool  compress       = false;
    // Compression threshold (dB) and ratio
    float compThreshDb   = 0.0f;
    float compRatio      = 1.0f;
};

//==============================================================================
/**
    Returns built-in presets.  Index order:
        0 = Bypass
        1 = The Sedan
        2 = The Phone
        3 = The Laptop
        4 = The Bluetooth Speaker

    EQ bands do the heavy lifting for frequency shaping.
    Convolution IRs are blended in subtly for realistic speaker/room coloring.
*/
inline std::vector<EnvironmentPreset> getBuiltInPresets()
{
    std::vector<EnvironmentPreset> presets;

    // 0 – Bypass (flat)
    {
        EnvironmentPreset p;
        p.name = "Bypass";
        presets.push_back (p);
    }

    // 1 – The Sedan
    //   EQ shapes the cabin character; IR adds subtle room feel.
    {
        EnvironmentPreset p;
        p.name             = "The Sedan";
        p.highPassFreq     = 35.0f;
        p.lowPassFreq      = 13000.0f;
        p.bands = {
            { 80.0f,   +2.0f, 0.8f },   // gentle cabin bass coupling
            { 250.0f,  +1.5f, 1.0f },    // slight boxy low-mid
            { 2000.0f, -1.5f, 1.0f },    // seat absorption dip
            { 8000.0f, -3.0f, 0.7f },    // upholstery damping
        };
        p.outputGainDb     = 0.5f;
        p.irResourceName   = BinaryData::sedan_ir_wav;
        p.irResourceSize   = BinaryData::sedan_ir_wavSize;
        p.irWetMix         = 0.20f;      // subtle cabin coloring
        p.stereoWidth      = 0.6f;
        p.earlyReflections = true;
        presets.push_back (p);
    }

    // 2 – The Phone
    //   EQ does the heavy lifting for tiny-speaker character; IR adds subtle flavor.
    {
        EnvironmentPreset p;
        p.name             = "The Phone";
        p.highPassFreq     = 300.0f;
        p.lowPassFreq      = 14000.0f;
        p.bands = {
            { 1500.0f, +1.5f, 1.2f },    // presence emphasis
            { 3500.0f, +2.5f, 2.0f },    // phone resonance peak
        };
        p.outputGainDb     = 2.0f;       // compensate for bass removal by HP
        p.irResourceName   = BinaryData::phone_ir_wav;
        p.irResourceSize   = BinaryData::phone_ir_wavSize;
        p.irWetMix         = 0.15f;      // light speaker coloring
        p.stereoWidth      = 0.0f;
        presets.push_back (p);
    }

    // 3 – The Laptop
    //   EQ shapes the thin/tinny character; IR adds subtle speaker coloring.
    {
        EnvironmentPreset p;
        p.name             = "The Laptop";
        p.highPassFreq     = 200.0f;
        p.lowPassFreq      = 16000.0f;
        p.bands = {
            { 1000.0f, +1.0f, 1.5f },    // tinny resonance
            { 2500.0f, +1.5f, 1.2f },    // laptop driver peak
        };
        p.outputGainDb     = 1.5f;       // compensate for bass removal by HP
        p.irResourceName   = BinaryData::laptop_ir_wav;
        p.irResourceSize   = BinaryData::laptop_ir_wavSize;
        p.irWetMix         = 0.15f;      // light speaker coloring
        p.stereoWidth      = 0.4f;
        presets.push_back (p);
    }

    // 4 – The Bluetooth Speaker
    //   EQ shapes the cheap-driver character; IR adds subtle coloring.
    {
        EnvironmentPreset p;
        p.name             = "The Bluetooth Speaker";
        p.highPassFreq     = 60.0f;
        p.lowPassFreq      = 15000.0f;
        p.bands = {
            { 100.0f,  +3.0f, 0.7f },    // bass enhancement
            { 3000.0f, +1.5f, 1.0f },    // presence push
        };
        p.outputGainDb     = 0.5f;
        p.irResourceName   = BinaryData::bt_speaker_ir_wav;
        p.irResourceSize   = BinaryData::bt_speaker_ir_wavSize;
        p.irWetMix         = 0.18f;      // subtle speaker coloring
        p.stereoWidth      = 0.0f;
        p.compress         = true;
        p.compThreshDb     = -12.0f;
        p.compRatio        = 4.0f;
        presets.push_back (p);
    }

    return presets;
}

//==============================================================================
/**
    Applies the full processing chain for a selected environment preset:
    HP -> LP -> Peak EQ -> Convolution IR (wet/dry blend) ->
    Early Reflections (car only) -> Stereo Width ->
    Compressor (BT only) -> Output Gain
*/
class EnvironmentProcessor
{
public:
    EnvironmentProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    /** Select a preset by index (0 = bypass). */
    void setPreset (int presetIndex);
    int  getPreset() const { return currentPresetIndex; }

private:
    void rebuildFilters();
    void loadIR (const char* data, int dataSize);

    int currentPresetIndex = 0;

    double sampleRate       = 44100.0;
    int    samplesPerBlock  = 512;
    int    numChannels      = 2;

    // IIR Filter chain (HP + LP + peak bands)
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoefs  = juce::dsp::IIR::Coefficients<float>;

    static constexpr int kMaxFilters = 10;
    std::array<juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefs>, kMaxFilters> filters;
    int activeFilterCount = 0;

    // Convolution engine
    juce::dsp::Convolution convolver;
    bool  convolverActive = false;
    float irWetMix        = 0.0f;

    // Early reflections (car cabin simulation)
    static constexpr int kMaxReflections = 5;
    struct ReflectionTap
    {
        int   delaySamples = 0;
        float gain         = 0.0f;
    };
    std::array<ReflectionTap, kMaxReflections> reflectionTaps;
    int numReflectionTaps = 0;
    bool earlyReflectionsActive = false;

    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;
    int delayBufferSize = 0;

    // Low-pass filter for early reflections (simulates absorption)
    juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefs> reflectionLPFilter;

    // Stereo width
    float stereoWidth = 1.0f;

    // Output gain
    juce::dsp::Gain<float> outputGain;

    // Compressor (BT speaker)
    juce::dsp::Compressor<float> compressor;
    bool compressorActive = false;

    std::vector<EnvironmentPreset> presets;
};
