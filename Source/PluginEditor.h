#pragma once

#include <JuceHeader.h>

#include "Parameters/Parameters.h"

class RemixSafeMasterAudioProcessor;

class RemixSafeMasterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                   private juce::Timer
{
public:
    explicit RemixSafeMasterAudioProcessorEditor(RemixSafeMasterAudioProcessor&);
    ~RemixSafeMasterAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    static void configureRotary(juce::Slider& slider, const juce::String& suffix);
    static void configureMeterLabel(juce::Label& label);
    static juce::String formatDecibels(float value, const juce::String& suffix);

    RemixSafeMasterAudioProcessor& processorReference;

    juce::Label title;
    juce::Label subtitle;
    juce::Slider inputTrim;
    juce::Slider ceiling;
    juce::Slider release;
    juce::Label inputTrimLabel;
    juce::Label ceilingLabel;
    juce::Label releaseLabel;
    juce::ComboBox quality;
    juce::Label qualityLabel;
    juce::ToggleButton autoRelease { "Auto Release" };
    juce::ToggleButton bypass { "Bypass" };

    juce::Label inputSampleMeter;
    juce::Label inputTruePeakMeter;
    juce::Label outputTruePeakMeter;
    juce::Label gainReductionMeter;
    juce::Label characterWarning;

    std::unique_ptr<SliderAttachment> inputTrimAttachment;
    std::unique_ptr<SliderAttachment> ceilingAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<ComboBoxAttachment> qualityAttachment;
    std::unique_ptr<ButtonAttachment> autoReleaseAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    float heldGainReduction = 0.0f;
    int peakHoldTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RemixSafeMasterAudioProcessorEditor)
};
