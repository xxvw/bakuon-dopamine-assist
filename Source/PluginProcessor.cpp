#include "PluginProcessor.h"
#include "PluginEditor.h"

RemixSafeMasterAudioProcessor::RemixSafeMasterAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
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
    destination.reset();
}

void RemixSafeMasterAudioProcessor::setStateInformation(const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RemixSafeMasterAudioProcessor();
}
