#include "EnvironmentProcessor.h"
#include <cmath>

//==============================================================================
EnvironmentProcessor::EnvironmentProcessor()
{
    presets = getBuiltInPresets();
}

void EnvironmentProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate      = spec.sampleRate;
    samplesPerBlock = static_cast<int> (spec.maximumBlockSize);
    numChannels     = static_cast<int> (spec.numChannels);

    for (auto& f : filters)
        f.prepare (spec);

    // Convolution engine
    convolver.prepare (spec);

    // Reflection LP filter
    reflectionLPFilter.prepare (spec);

    // Early reflections delay buffer — enough for ~15ms at any sample rate
    delayBufferSize = static_cast<int> (sampleRate * 0.015);
    delayBuffer.setSize (numChannels, delayBufferSize);
    delayBuffer.clear();
    delayWritePos = 0;

    outputGain.prepare (spec);
    compressor.prepare (spec);

    rebuildFilters();
}

void EnvironmentProcessor::reset()
{
    for (auto& f : filters)
        f.reset();

    convolver.reset();
    reflectionLPFilter.reset();
    delayBuffer.clear();
    delayWritePos = 0;
    outputGain.reset();
    compressor.reset();
}

void EnvironmentProcessor::setPreset (int idx)
{
    if (idx < 0 || idx >= static_cast<int> (presets.size()))
        idx = 0;

    if (idx != currentPresetIndex)
    {
        currentPresetIndex = idx;
        rebuildFilters();
    }
}

//==============================================================================
void EnvironmentProcessor::loadIR (const char* data, int dataSize)
{
    if (data == nullptr || dataSize == 0)
    {
        convolverActive = false;
        return;
    }

    convolver.loadImpulseResponse (data, static_cast<size_t> (dataSize),
                                   juce::dsp::Convolution::Stereo::yes,
                                   juce::dsp::Convolution::Trim::yes,
                                   0);  // 0 = use full IR length
    convolverActive = true;
}

//==============================================================================
void EnvironmentProcessor::rebuildFilters()
{
    // Reset all filters
    for (auto& f : filters)
        f.reset();

    activeFilterCount       = 0;
    compressorActive        = false;
    convolverActive         = false;
    earlyReflectionsActive  = false;
    irWetMix                = 0.0f;
    stereoWidth             = 1.0f;
    numReflectionTaps       = 0;

    const auto& preset = presets[static_cast<size_t> (currentPresetIndex)];

    if (currentPresetIndex == 0)
    {
        // Bypass – no processing
        outputGain.setGainDecibels (0.0f);
        return;
    }

    // ---- IIR Filters ----

    // High-pass
    if (activeFilterCount < kMaxFilters)
    {
        *filters[static_cast<size_t> (activeFilterCount)].state =
            *IIRCoefs::makeHighPass (sampleRate, preset.highPassFreq, 0.707f);
        activeFilterCount++;
    }

    // Low-pass
    if (activeFilterCount < kMaxFilters)
    {
        *filters[static_cast<size_t> (activeFilterCount)].state =
            *IIRCoefs::makeLowPass (sampleRate, preset.lowPassFreq, 0.707f);
        activeFilterCount++;
    }

    // Peak EQ bands
    for (const auto& band : preset.bands)
    {
        if (activeFilterCount >= kMaxFilters)
            break;

        *filters[static_cast<size_t> (activeFilterCount)].state =
            *IIRCoefs::makePeakFilter (sampleRate, band.freq, band.q,
                                       juce::Decibels::decibelsToGain (band.gainDb));
        activeFilterCount++;
    }

    // ---- Convolution IR ----
    loadIR (preset.irResourceName, preset.irResourceSize);
    irWetMix = preset.irWetMix;

    // ---- Early Reflections (car cabin only) ----
    if (preset.earlyReflections)
    {
        earlyReflectionsActive = true;

        // Delay taps simulating car cabin reflections:
        // windshield, dashboard, side windows, rear window, headliner
        struct TapSpec { float delayMs; float gain; };
        const TapSpec tapSpecs[kMaxReflections] = {
            { 1.2f, 0.35f },   // windshield (closest, strongest)
            { 2.1f, 0.25f },   // dashboard
            { 3.0f, 0.18f },   // left side window
            { 4.3f, 0.12f },   // right side window
            { 5.5f, 0.08f },   // rear window (farthest, weakest)
        };

        numReflectionTaps = kMaxReflections;
        for (int i = 0; i < kMaxReflections; ++i)
        {
            reflectionTaps[static_cast<size_t> (i)].delaySamples =
                static_cast<int> (tapSpecs[i].delayMs * 0.001f * static_cast<float> (sampleRate));
            reflectionTaps[static_cast<size_t> (i)].gain = tapSpecs[i].gain;
        }

        // LP filter on reflections to simulate high-frequency absorption
        *reflectionLPFilter.state =
            *IIRCoefs::makeLowPass (sampleRate, 6000.0f, 0.707f);
        reflectionLPFilter.reset();

        delayBuffer.clear();
        delayWritePos = 0;
    }

    // ---- Stereo Width ----
    stereoWidth = preset.stereoWidth;

    // ---- Output gain ----
    outputGain.setGainDecibels (preset.outputGainDb);

    // ---- Compressor ----
    if (preset.compress)
    {
        compressorActive = true;
        compressor.setThreshold (preset.compThreshDb);
        compressor.setRatio (preset.compRatio);
        compressor.setAttack (10.0f);
        compressor.setRelease (100.0f);
    }
}

//==============================================================================
void EnvironmentProcessor::process (juce::AudioBuffer<float>& buffer)
{
    if (currentPresetIndex == 0)
        return; // bypass

    const int numSamples = buffer.getNumSamples();
    const int channels   = buffer.getNumChannels();

    // ---- 1. IIR Filters (HP -> LP -> Peak EQ) ----
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);

        for (int i = 0; i < activeFilterCount; ++i)
            filters[static_cast<size_t> (i)].process (context);
    }

    // ---- 2. Convolution IR (wet/dry blend) ----
    if (convolverActive && irWetMix > 0.0f)
    {
        // Save the dry (post-EQ) signal
        juce::AudioBuffer<float> dryBuffer (channels, numSamples);
        for (int ch = 0; ch < channels; ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        // Process through convolution (replaces buffer with wet signal)
        {
            juce::dsp::AudioBlock<float> block (buffer);
            juce::dsp::ProcessContextReplacing<float> context (block);
            convolver.process (context);
        }

        // Blend: output = dry * (1 - wet) + convolved * wet
        const float dryGain = 1.0f - irWetMix;
        const float wetGain = irWetMix;

        for (int ch = 0; ch < channels; ++ch)
        {
            auto* out     = buffer.getWritePointer (ch);
            const auto* dry = dryBuffer.getReadPointer (ch);

            for (int s = 0; s < numSamples; ++s)
                out[s] = dry[s] * dryGain + out[s] * wetGain;
        }
    }

    // ---- 3. Early Reflections (car cabin only) ----
    if (earlyReflectionsActive && numReflectionTaps > 0)
    {
        // Create a temp buffer to accumulate reflections
        juce::AudioBuffer<float> reflectionBuf (channels, numSamples);
        reflectionBuf.clear();

        for (int s = 0; s < numSamples; ++s)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                // Write current sample to delay buffer
                delayBuffer.setSample (ch, delayWritePos, buffer.getSample (ch, s));

                // Sum tapped reflections
                float reflected = 0.0f;
                for (int t = 0; t < numReflectionTaps; ++t)
                {
                    int readPos = delayWritePos - reflectionTaps[static_cast<size_t> (t)].delaySamples;
                    if (readPos < 0)
                        readPos += delayBufferSize;

                    reflected += delayBuffer.getSample (ch, readPos)
                                 * reflectionTaps[static_cast<size_t> (t)].gain;
                }

                reflectionBuf.setSample (ch, s, reflected);
            }

            delayWritePos = (delayWritePos + 1) % delayBufferSize;
        }

        // LP filter the reflections to simulate absorption
        {
            juce::dsp::AudioBlock<float> refBlock (reflectionBuf);
            juce::dsp::ProcessContextReplacing<float> refContext (refBlock);
            reflectionLPFilter.process (refContext);
        }

        // Add reflections to signal
        for (int ch = 0; ch < channels; ++ch)
            buffer.addFrom (ch, 0, reflectionBuf, ch, 0, numSamples);
    }

    // ---- 4. Stereo Width (mid-side processing) ----
    if (stereoWidth < 1.0f && channels >= 2)
    {
        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);

        for (int s = 0; s < numSamples; ++s)
        {
            const float mid  = (left[s] + right[s]) * 0.5f;
            const float side = (left[s] - right[s]) * 0.5f;

            const float scaledSide = side * stereoWidth;

            left[s]  = mid + scaledSide;
            right[s] = mid - scaledSide;
        }
    }

    // ---- 5. Compressor (BT speaker) ----
    if (compressorActive)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        compressor.process (context);
    }

    // ---- 6. Output gain trim ----
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        outputGain.process (context);
    }
}
