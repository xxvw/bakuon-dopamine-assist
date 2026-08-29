#include "PluginProcessor.h"

#include <JuceHeader.h>

#include <cmath>
#include <memory>
#include <numbers>

int main(int argumentCount, char* arguments[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    RemixSafeMasterAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> audio(2, 2048);
    for (int sample = 0; sample < audio.getNumSamples(); ++sample)
    {
        const auto wave = static_cast<float>(
            2.0 * std::sin(2.0 * std::numbers::pi * 7000.0
                           * static_cast<double>(sample) / 48000.0));
        audio.setSample(0, sample, wave);
        audio.setSample(1, sample, -0.5f * wave);
    }

    juce::MidiBuffer midi;
    processor.processBlock(audio, midi);

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    if (editor == nullptr)
        return 1;

    editor->setSize(900, 620);

    const auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);
    if (! snapshot.isValid())
        return 2;

    const auto output = argumentCount > 1
        ? juce::File(arguments[1])
        : juce::File::getSpecialLocation(juce::File::tempDirectory)
              .getChildFile("remix-safe-master-ui.png");

    juce::MemoryOutputStream encoded;
    juce::PNGImageFormat png;
    if (! png.writeImageToStream(snapshot, encoded))
        return 3;
    if (! output.replaceWithData(encoded.getData(), encoded.getDataSize()))
        return 4;

    return 0;
}
