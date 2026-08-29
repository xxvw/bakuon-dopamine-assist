#pragma once

#include <JuceHeader.h>

class RemixSafeMasterAudioProcessor;

class RemixSafeMasterAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit RemixSafeMasterAudioProcessorEditor(RemixSafeMasterAudioProcessor&);
    ~RemixSafeMasterAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::Label title;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RemixSafeMasterAudioProcessorEditor)
};
