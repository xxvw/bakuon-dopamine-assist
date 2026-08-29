#pragma once

#include <JuceHeader.h>

#include "Parameters/Parameters.h"

#include <array>

class DopamineLookAndFeel;
class RemixSafeMasterAudioProcessor;

class RemixSafeMasterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                   private juce::Timer
{
public:
    explicit RemixSafeMasterAudioProcessorEditor(RemixSafeMasterAudioProcessor&);
    ~RemixSafeMasterAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void drawMeter(juce::Graphics&,
                   juce::Rectangle<float>,
                   const juce::String& name,
                   float value,
                   float minimum,
                   float maximum,
                   juce::Colour colour,
                   bool gainReduction) const;
    static void configureRotary(juce::Slider& slider,
                                const juce::String& suffix,
                                juce::Colour colour,
                                const juce::String& accessibleName,
                                const juce::String& tooltip);
    static void configureControlLabel(juce::Label& label,
                                      const juce::String& title,
                                      const juce::String& step);
    static void configureMeterValueLabel(juce::Label& label);
    static juce::String formatDecibels(float value, const juce::String& suffix);
    static float updatePeakDisplay(float current, float target, float decayPerTick) noexcept;

    RemixSafeMasterAudioProcessor& processorReference;
    std::unique_ptr<DopamineLookAndFeel> dopamineLookAndFeel;

    juce::Label productBadge;
    juce::Label title;
    juce::Label subtitle;
    juce::Label liveBadge;

    juce::Slider inputTrim;
    juce::Slider ceiling;
    juce::Slider release;
    juce::Label inputTrimLabel;
    juce::Label ceilingLabel;
    juce::Label releaseLabel;
    juce::ComboBox quality;
    juce::Label qualityLabel;
    juce::Label qualityHint;
    juce::ToggleButton autoRelease;
    juce::ToggleButton bypass;

    juce::Label meterSectionTitle;
    juce::Label inputSampleMeter;
    juce::Label inputTruePeakMeter;
    juce::Label outputTruePeakMeter;
    juce::Label gainReductionMeter;
    juce::Label characterWarning;
    juce::Label footerHint;
    juce::TooltipWindow tooltipWindow;

    std::unique_ptr<SliderAttachment> inputTrimAttachment;
    std::unique_ptr<SliderAttachment> ceilingAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<ComboBoxAttachment> qualityAttachment;
    std::unique_ptr<ButtonAttachment> autoReleaseAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    std::array<juce::Rectangle<float>, 4> meterPaintBounds {};
    float displayInputSample = -72.0f;
    float displayInputTruePeak = -72.0f;
    float displayOutputTruePeak = -72.0f;
    float displayGainReduction = 0.0f;
    float heldGainReduction = 0.0f;
    float animationPhase = 0.0f;
    int peakHoldTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RemixSafeMasterAudioProcessorEditor)
};
