#include "NoiseGenerator.h"

//==============================================================================
NoiseGenerator::NoiseGenerator() {}

void NoiseGenerator::prepare (double sr, int /*samplesPerBlock*/)
{
    currentSampleRate = sr;

    // Road noise low-pass  – keep rumble below ~400 Hz
    roadLP.setCoefficients (400.0f, sr);

    // AC hum band: LP at 180 Hz, HP at 80 Hz -> narrow band around 120 Hz
    humLP.setCoefficients (180.0f, sr);
    humHP.setCoefficients (80.0f,  sr);

    // City ambience mid-range texture
    cityLP.setCoefficients (2000.0f, sr);

    reset();
}

void NoiseGenerator::reset()
{
    pink0 = pink1 = pink2 = pink3 = pink4 = pink5 = pink6 = 0.0f;
    roadLP.reset();
    humLP.reset();
    humHP.reset();
    cityLP.reset();
}

//==============================================================================
float NoiseGenerator::generatePinkSample()
{
    // Paul Kellet's economy pink noise approximation
    const float white = rng.nextFloat() * 2.0f - 1.0f;

    pink0 = 0.99886f * pink0 + white * 0.0555179f;
    pink1 = 0.99332f * pink1 + white * 0.0750759f;
    pink2 = 0.96900f * pink2 + white * 0.1538520f;
    pink3 = 0.86650f * pink3 + white * 0.3104856f;
    pink4 = 0.55000f * pink4 + white * 0.5329522f;
    pink5 = -0.7616f * pink5 - white * 0.0168980f;

    float pink = pink0 + pink1 + pink2 + pink3 + pink4 + pink5 + pink6 + white * 0.5362f;
    pink6 = white * 0.115926f;

    return pink * 0.11f; // normalize
}

//==============================================================================
void NoiseGenerator::process (juce::AudioBuffer<float>& buffer, float amount)
{
    if (amount <= 0.0001f)
        return;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int s = 0; s < numSamples; ++s)
    {
        float pinkSample = generatePinkSample();
        float white = rng.nextFloat() * 2.0f - 1.0f;

        // City noise: road rumble + engine/AC hum + mid-range ambience
        float roadSample = roadLP.processSample (pinkSample);
        float humSample  = humLP.processSample (white);
        humSample = humSample - humHP.processSample (humSample); // crude band-pass
        float citySample = cityLP.processSample (pinkSample * 0.5f + white * 0.5f);

        float noise = roadSample * 0.45f + humSample * 0.30f + citySample * 0.25f;

        // Scale by amount  (amount 0..1 maps to roughly -60 dB .. -12 dB)
        float gain = amount * amount * 0.25f;   // quadratic taper feels more natural
        noise *= gain;

        // Add to every channel
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[s] += noise;
    }
}

//==============================================================================
void NoiseGenerator::OnePole::setCoefficients (float cutoff, double sr)
{
    float w = juce::MathConstants<float>::twoPi * cutoff / static_cast<float> (sr);
    float c = std::cos (w);
    a1 = -(c - 1.0f) / (c + 1.0f);      // simple one-pole LP approximation
    b0 = (1.0f + a1) * 0.5f;
}

float NoiseGenerator::OnePole::processSample (float x)
{
    float y = b0 * x - a1 * z1;
    z1 = y;
    return y;
}
