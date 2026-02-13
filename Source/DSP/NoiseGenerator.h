#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
    Generates city background noise to blend into the output.
    Driven by a single "amount" parameter (0 = silent, 1 = full).

    The noise is a mix of:
      - Filtered pink noise  (simulates road rumble / traffic)
      - Narrow-band hum      (simulates AC / engine drone)
      - Occasional broader spectrum (city ambience)
*/
class NoiseGenerator
{
public:
    NoiseGenerator();

    void prepare (double sampleRate, int samplesPerBlock);
    void process (juce::AudioBuffer<float>& buffer, float amount);
    void reset();

private:
    /** Simple one-pole filter for colouring white noise. */
    struct OnePole
    {
        float b0 = 0.0f, a1 = 0.0f, z1 = 0.0f;
        void  setCoefficients (float cutoff, double sr);
        float processSample (float x);
        void  reset() { z1 = 0.0f; }
    };

    juce::Random rng;
    double currentSampleRate = 44100.0;

    // Pink noise approximation via Paul Kellet's method
    float pink0 = 0.0f, pink1 = 0.0f, pink2 = 0.0f,
          pink3 = 0.0f, pink4 = 0.0f, pink5 = 0.0f, pink6 = 0.0f;
    float generatePinkSample();

    // Low-pass to shape road noise
    OnePole roadLP;
    // Band-pass for AC / engine hum (~120 Hz)
    OnePole humLP, humHP;
    // Mid-range city ambience
    OnePole cityLP;
};
