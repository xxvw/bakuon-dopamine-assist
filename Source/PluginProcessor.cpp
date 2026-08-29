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

void RemixSafeMasterAudioProcessor::prepareToPlay(double, int)
{
}

void RemixSafeMasterAudioProcessor::releaseResources()
{
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
void RemixSafeMasterAudioProcessor::process(juce::AudioBuffer<SampleType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

void RemixSafeMasterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer&)
{
    process(buffer);
}

void RemixSafeMasterAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                                  juce::MidiBuffer&)
{
    process(buffer);
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
