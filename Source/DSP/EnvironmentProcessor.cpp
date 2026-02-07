#include "EnvironmentProcessor.h"

//==============================================================================
EnvironmentProcessor::EnvironmentProcessor()
{
    presets = getBuiltInPresets();
}

void EnvironmentProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate      = spec.sampleRate;
    samplesPerBlock = static_cast<int> (spec.maximumBlockSize);

    for (auto& f : filters)
        f.prepare (spec);

    outputGain.prepare (spec);
    compressor.prepare (spec);

    rebuildFilters();
}

void EnvironmentProcessor::reset()
{
    for (auto& f : filters)
        f.reset();

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

void EnvironmentProcessor::setWindowsDown (bool enabled)
{
    if (enabled != windowsDown)
    {
        windowsDown = enabled;
        rebuildFilters();
    }
}

//==============================================================================
void EnvironmentProcessor::rebuildFilters()
{
    // Reset all filters
    for (auto& f : filters)
        f.reset();

    activeFilterCount  = 0;
    compressorActive   = false;

    const auto& preset = presets[static_cast<size_t> (currentPresetIndex)];

    if (currentPresetIndex == 0)
    {
        // Bypass – no processing
        outputGain.setGainDecibels (0.0f);
        return;
    }

    // High-pass
    float hpFreq = preset.highPassFreq;

    // "Windows Down" mode: push the high-pass up dramatically to kill bass
    if (windowsDown)
        hpFreq = juce::jmax (hpFreq, 500.0f);

    if (activeFilterCount < kMaxFilters)
    {
        *filters[static_cast<size_t> (activeFilterCount)].state =
            *IIRCoefs::makeHighPass (sampleRate, hpFreq, 0.707f);
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
            *IIRCoefs::makePeakFilter (sampleRate, band.freq, band.q, juce::Decibels::decibelsToGain (band.gainDb));
        activeFilterCount++;
    }

    // Output gain
    outputGain.setGainDecibels (preset.outputGainDb);

    // Compressor
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

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    // Apply filters
    for (int i = 0; i < activeFilterCount; ++i)
        filters[static_cast<size_t> (i)].process (context);

    // Compressor (BT speaker)
    if (compressorActive)
        compressor.process (context);

    // Output gain trim
    outputGain.process (context);

    // Mono-sum if the preset demands it
    const auto& preset = presets[static_cast<size_t> (currentPresetIndex)];
    if (preset.monoSum && buffer.getNumChannels() >= 2)
    {
        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);
        const int numSamples = buffer.getNumSamples();

        for (int s = 0; s < numSamples; ++s)
        {
            const float mono = (left[s] + right[s]) * 0.5f;
            left[s]  = mono;
            right[s] = mono;
        }
    }
}
