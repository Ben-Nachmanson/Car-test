#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
    Defines the EQ / processing profile for a single listening environment.
    Each profile is a set of IIR filter specs + gain + optional mono summing.
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
    // Whether to mono-sum the output
    bool  monoSum        = false;
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
*/
inline std::vector<EnvironmentPreset> getBuiltInPresets()
{
    std::vector<EnvironmentPreset> presets;

    // 0 – Bypass (flat)
    presets.push_back ({ "Bypass", 20.0f, 20000.0f, {}, 0.0f, false, false, 0.0f, 1.0f });

    // 1 – The Sedan
    //   Boxy car interior: hyped low-mids from the cabin resonance,
    //   rolled-off highs from upholstery absorption, slight bass boost
    //   from the enclosed trunk/cabin coupling.
    {
        EnvironmentPreset p;
        p.name         = "The Sedan";
        p.highPassFreq = 35.0f;
        p.lowPassFreq  = 12000.0f;
        p.bands = {
            { 80.0f,   +4.0f, 0.8f },   // cabin bass coupling
            { 250.0f,  +3.0f, 1.0f },    // boxy low-mid resonance
            { 500.0f,  +1.5f, 1.2f },    // dashboard reflection
            { 2000.0f, -2.0f, 1.0f },    // seat absorption dip
            { 6000.0f, -5.0f, 0.7f },    // upholstery damping
            { 10000.0f,-8.0f, 0.7f },    // severe high-end rolloff
        };
        p.outputGainDb = -2.0f;
        presets.push_back (p);
    }

    // 2 – The Phone
    //   Smartphone speaker: tiny driver, almost no bass,
    //   peaky upper-mids, harsh resonance around 3-4 kHz.
    {
        EnvironmentPreset p;
        p.name         = "The Phone";
        p.highPassFreq = 300.0f;
        p.lowPassFreq  = 14000.0f;
        p.bands = {
            { 800.0f,  +2.0f, 1.5f },    // small-driver resonance
            { 1500.0f, +3.0f, 1.2f },    // presence
            { 3500.0f, +5.0f, 2.0f },    // harsh peak (phone resonance)
            { 5000.0f, +2.0f, 1.5f },    // upper presence
            { 8000.0f, -3.0f, 1.0f },    // air roll-off
        };
        p.outputGainDb = -4.0f;
        p.monoSum      = true;
        presets.push_back (p);
    }

    // 3 – The Laptop
    //   Built-in laptop speakers: thin, tinny, some stereo from
    //   spaced drivers, resonant mid-range, no low-end body.
    {
        EnvironmentPreset p;
        p.name         = "The Laptop";
        p.highPassFreq = 200.0f;
        p.lowPassFreq  = 16000.0f;
        p.bands = {
            { 400.0f,  -2.0f, 1.0f },    // thin body
            { 1000.0f, +2.0f, 1.5f },    // tinny resonance
            { 2500.0f, +3.0f, 1.2f },    // laptop driver peak
            { 5000.0f, +1.0f, 1.0f },    // clarity
            { 8000.0f, -2.0f, 0.8f },    // slight air loss
        };
        p.outputGainDb = -3.0f;
        presets.push_back (p);
    }

    // 4 – The Bluetooth Speaker
    //   Mono, compressed, bass-boosted (DSP-enhanced bass in real life).
    {
        EnvironmentPreset p;
        p.name         = "The Bluetooth Speaker";
        p.highPassFreq = 60.0f;
        p.lowPassFreq  = 15000.0f;
        p.bands = {
            { 100.0f,  +6.0f, 0.7f },    // bass enhancement
            { 200.0f,  +3.0f, 1.0f },    // upper bass boost
            { 800.0f,  -1.0f, 1.2f },    // slight scoop
            { 3000.0f, +2.0f, 1.0f },    // presence push
            { 7000.0f, -4.0f, 0.8f },    // cheap driver roll-off
        };
        p.outputGainDb = -1.0f;
        p.monoSum      = true;
        p.compress     = true;
        p.compThreshDb = -12.0f;
        p.compRatio    = 4.0f;
        presets.push_back (p);
    }

    return presets;
}

//==============================================================================
/**
    Applies the EQ + processing chain for a selected environment preset.
    Handles filter coefficients, gain, mono-summing, and simple compression.
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

    /** "Windows Down" mode – applies extra high-pass to kill bass. */
    void setWindowsDown (bool enabled);
    bool getWindowsDown() const { return windowsDown; }

private:
    void rebuildFilters();

    int currentPresetIndex = 0;
    bool windowsDown = false;

    double sampleRate = 44100.0;
    int    samplesPerBlock = 512;

    // Filter chain
    using IIRFilter = juce::dsp::IIR::Filter<float>;
    using IIRCoefs  = juce::dsp::IIR::Coefficients<float>;

    // We keep a fixed number of filter slots – enough for HP + LP + 6 peak bands + windows-down HP
    static constexpr int kMaxFilters = 10;
    std::array<juce::dsp::ProcessorDuplicator<IIRFilter, IIRCoefs>, kMaxFilters> filters;
    int activeFilterCount = 0;

    juce::dsp::Gain<float> outputGain;

    // Simple compressor for BT speaker preset
    juce::dsp::Compressor<float> compressor;
    bool compressorActive = false;

    std::vector<EnvironmentPreset> presets;
};
