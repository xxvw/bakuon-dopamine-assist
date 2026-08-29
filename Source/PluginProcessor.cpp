#include "PluginProcessor.h"
#include "PluginEditor.h"

RemixSafeMasterAudioProcessor::RemixSafeMasterAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this,
                 nullptr,
                 bakuon::parameters::stateTreeType,
                 bakuon::parameters::createParameterLayout()),
      rawParameters(bakuon::parameters::getRawParameterPointers(parameters))
{
    setProcessingPrecision(ProcessingPrecision::singlePrecision);
}

void RemixSafeMasterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    limiter.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    setLatencySamples(limiter.getLatencySamples());
}

void RemixSafeMasterAudioProcessor::releaseResources()
{
    limiter.reset();
}

bool RemixSafeMasterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    const auto supported = input == juce::AudioChannelSet::mono()
                        || input == juce::AudioChannelSet::stereo();
    return supported && input == output;
}

template <typename SampleType>
void RemixSafeMasterAudioProcessor::process(juce::AudioBuffer<SampleType>& buffer,
                                            bool forceBypass)
{
    juce::ScopedNoDenormals noDenormals;
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    const auto quality = rawParameters.quality->load(std::memory_order_relaxed) >= 0.5f
        ? bakuon::dsp::OversamplingStage::highFactor
        : bakuon::dsp::OversamplingStage::normalFactor;
    limiter.setParameters({
        rawParameters.inputTrim->load(std::memory_order_relaxed),
        rawParameters.ceiling->load(std::memory_order_relaxed),
        rawParameters.release->load(std::memory_order_relaxed),
        rawParameters.autoRelease->load(std::memory_order_relaxed) >= 0.5f,
        quality,
        forceBypass || rawParameters.bypass->load(std::memory_order_relaxed) >= 0.5f
    });

    limiter.beginBlock();
    const auto channels = std::min(buffer.getNumChannels(),
                                   bakuon::dsp::OversamplingStage::maxChannels);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        std::array<double, bakuon::dsp::OversamplingStage::maxChannels> input {};
        for (int channel = 0; channel < channels; ++channel)
            input[static_cast<std::size_t>(channel)] =
                static_cast<double>(buffer.getSample(channel, sample));

        const auto output = limiter.processFrame(input, channels);
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample(channel,
                             sample,
                             static_cast<SampleType>(output[static_cast<std::size_t>(channel)]));
    }

    metering.publish(limiter.endBlock());
}

void RemixSafeMasterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer&)
{
    process(buffer, false);
}

void RemixSafeMasterAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                                  juce::MidiBuffer&)
{
    process(buffer, false);
}

void RemixSafeMasterAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer,
                                                         juce::MidiBuffer&)
{
    process(buffer, true);
}

void RemixSafeMasterAudioProcessor::processBlockBypassed(juce::AudioBuffer<double>& buffer,
                                                         juce::MidiBuffer&)
{
    process(buffer, true);
}

juce::AudioProcessorEditor* RemixSafeMasterAudioProcessor::createEditor()
{
    return new RemixSafeMasterAudioProcessorEditor(*this);
}

void RemixSafeMasterAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    state.setProperty("schemaVersion", bakuon::parameters::stateSchemaVersion, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void RemixSafeMasterAudioProcessor::setStateInformation(const void* data, int size)
{
    const auto xml = getXmlFromBinary(data, size);
    if (xml == nullptr || ! xml->hasTagName(parameters.state.getType()))
        return;

    auto restored = juce::ValueTree::fromXml(*xml);
    if (! restored.isValid())
        return;

    if (! restored.hasProperty("schemaVersion"))
        restored.setProperty("schemaVersion", 0, nullptr);

    parameters.replaceState(restored);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RemixSafeMasterAudioProcessor();
}
