#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
    Generates background noise (road noise, AC hum, wind) to blend into the
    output.  Driven by a single "amount" parameter (0 = silent, 1 = full).

    The noise is a mix of:
      - Filtered pink noise  (simulates road rumble)
      - Narrow-band hum      (simulates AC / fan drone)
    When "windows down" mode is active, the character shifts to a broader
    wind-rush noise with more high-frequency content.
*/
class NoiseGenerator
{
public:
    NoiseGenerator();

    void prepare (double sampleRate, int samplesPerBlock);
    void process (juce::AudioBuffer<float>& buffer, float amount, bool windowsDown);
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
    // Band-pass for AC hum (~120 Hz)
    OnePole humLP, humHP;

    // Wind noise high-pass
    OnePole windHP;
};
