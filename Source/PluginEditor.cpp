#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <algorithm>
#include <array>

namespace
{
const auto background = juce::Colour(0xff14181e);
const auto panel = juce::Colour(0xff202832);
const auto accent = juce::Colour(0xff60bedc);
const auto text = juce::Colour(0xffe8edf2);
const auto warning = juce::Colour(0xffffb24a);
}

RemixSafeMasterAudioProcessorEditor::RemixSafeMasterAudioProcessorEditor(
    RemixSafeMasterAudioProcessor& audioProcessor)
    : AudioProcessorEditor(audioProcessor),
      processorReference(audioProcessor)
{
    title.setText("Remix Safe Master", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centredLeft);
    title.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, text);

    subtitle.setText("KEEP THE DAMAGE. PROTECT THE OUTPUT.", juce::dontSendNotification);
    subtitle.setJustificationType(juce::Justification::centredLeft);
    subtitle.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    subtitle.setColour(juce::Label::textColourId, accent);

    configureRotary(inputTrim, " dB");
    configureRotary(ceiling, " dBTP");
    configureRotary(release, " ms");

    inputTrimLabel.setText("INPUT TRIM", juce::dontSendNotification);
    ceilingLabel.setText("CEILING", juce::dontSendNotification);
    releaseLabel.setText("RELEASE", juce::dontSendNotification);
    qualityLabel.setText("QUALITY", juce::dontSendNotification);
    for (auto* label : { &inputTrimLabel, &ceilingLabel, &releaseLabel, &qualityLabel })
    {
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, text.withAlpha(0.8f));
        label->setFont(juce::FontOptions(11.0f, juce::Font::bold));
    }

    quality.addItem("Normal · 4x", 1);
    quality.addItem("High · 8x", 2);
    quality.setJustificationType(juce::Justification::centred);

    configureMeterLabel(inputSampleMeter);
    configureMeterLabel(inputTruePeakMeter);
    configureMeterLabel(outputTruePeakMeter);
    configureMeterLabel(gainReductionMeter);
    characterWarning.setJustificationType(juce::Justification::centred);
    characterWarning.setColour(juce::Label::textColourId, warning);
    characterWarning.setFont(juce::FontOptions(12.0f, juce::Font::bold));

    const std::array<juce::Component*, 17> components {
        &title, &subtitle, &inputTrim, &ceiling, &release,
        &inputTrimLabel, &ceilingLabel, &releaseLabel, &quality, &qualityLabel,
        &autoRelease, &bypass, &inputSampleMeter, &inputTruePeakMeter,
        &outputTruePeakMeter, &gainReductionMeter, &characterWarning
    };
    for (auto* component : components)
        addAndMakeVisible(component);

    auto& state = processorReference.parameters;
    inputTrimAttachment = std::make_unique<SliderAttachment>(
        state, bakuon::parameters::ids::inputTrim, inputTrim);
    ceilingAttachment = std::make_unique<SliderAttachment>(
        state, bakuon::parameters::ids::ceiling, ceiling);
    releaseAttachment = std::make_unique<SliderAttachment>(
        state, bakuon::parameters::ids::release, release);
    qualityAttachment = std::make_unique<ComboBoxAttachment>(
        state, bakuon::parameters::ids::quality, quality);
    autoReleaseAttachment = std::make_unique<ButtonAttachment>(
        state, bakuon::parameters::ids::autoRelease, autoRelease);
    bypassAttachment = std::make_unique<ButtonAttachment>(
        state, bakuon::parameters::ids::bypass, bypass);

    setResizable(true, true);
    setResizeLimits(640, 430, 1000, 720);
    setSize(760, 520);
    startTimerHz(30);
}

void RemixSafeMasterAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background);

    const auto bounds = getLocalBounds().toFloat().reduced(18.0f);
    graphics.setColour(panel);
    graphics.fillRoundedRectangle(bounds.withHeight(72.0f), 10.0f);
    graphics.fillRoundedRectangle(bounds.withTrimmedTop(88.0f).withHeight(205.0f), 10.0f);
    graphics.fillRoundedRectangle(bounds.withTrimmedTop(309.0f), 10.0f);

    graphics.setColour(accent.withAlpha(0.7f));
    graphics.drawRoundedRectangle(bounds, 12.0f, 1.0f);
}

void RemixSafeMasterAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(28);
    auto header = bounds.removeFromTop(62);
    title.setBounds(header.removeFromTop(36));
    subtitle.setBounds(header);

    bounds.removeFromTop(28);
    auto controls = bounds.removeFromTop(185);
    constexpr int columnCount = 5;
    const auto columnWidth = controls.getWidth() / columnCount;

    const auto setRotaryBounds = [columnWidth](juce::Rectangle<int> area,
                                                juce::Label& label,
                                                juce::Slider& slider)
    {
        area.setWidth(columnWidth);
        label.setBounds(area.removeFromTop(24));
        slider.setBounds(area.reduced(6));
    };

    auto column = controls;
    setRotaryBounds(column.removeFromLeft(columnWidth), inputTrimLabel, inputTrim);
    setRotaryBounds(column.removeFromLeft(columnWidth), ceilingLabel, ceiling);
    setRotaryBounds(column.removeFromLeft(columnWidth), releaseLabel, release);

    auto qualityArea = column.removeFromLeft(columnWidth).reduced(8);
    qualityLabel.setBounds(qualityArea.removeFromTop(24));
    qualityArea.removeFromTop(32);
    quality.setBounds(qualityArea.removeFromTop(34));
    autoRelease.setBounds(qualityArea.removeFromTop(34));

    auto bypassArea = column.reduced(18);
    bypass.setBounds(bypassArea.withSizeKeepingCentre(100, 42));

    bounds.removeFromTop(30);
    auto meterArea = bounds.removeFromTop(116);
    const auto meterWidth = meterArea.getWidth() / 4;
    inputSampleMeter.setBounds(meterArea.removeFromLeft(meterWidth).reduced(5));
    inputTruePeakMeter.setBounds(meterArea.removeFromLeft(meterWidth).reduced(5));
    outputTruePeakMeter.setBounds(meterArea.removeFromLeft(meterWidth).reduced(5));
    gainReductionMeter.setBounds(meterArea.reduced(5));
    characterWarning.setBounds(bounds.removeFromTop(36));
}

void RemixSafeMasterAudioProcessorEditor::timerCallback()
{
    const auto meters = processorReference.getMeterValues();
    inputSampleMeter.setText("INPUT SP\n" + formatDecibels(meters.inputSamplePeakDb, " dBFS"),
                             juce::dontSendNotification);
    inputTruePeakMeter.setText("INPUT TP\n" + formatDecibels(meters.inputTruePeakDb, " dBTP"),
                               juce::dontSendNotification);
    outputTruePeakMeter.setText("OUTPUT TP\n" + formatDecibels(meters.outputTruePeakDb, " dBTP"),
                                juce::dontSendNotification);

    if (meters.peakGainReductionDb >= heldGainReduction)
    {
        heldGainReduction = meters.peakGainReductionDb;
        peakHoldTicks = 30;
    }
    else if (peakHoldTicks > 0)
    {
        --peakHoldTicks;
    }
    else
    {
        heldGainReduction = std::max(meters.currentGainReductionDb,
                                     heldGainReduction - 0.15f);
    }

    gainReductionMeter.setText("GAIN REDUCTION\n"
                               + juce::String(meters.currentGainReductionDb, 1)
                               + " / " + juce::String(heldGainReduction, 1) + " dB",
                               juce::dontSendNotification);
    characterWarning.setText(heldGainReduction > 3.0f
                                 ? "CHARACTER CHANGE POSSIBLE · GR EXCEEDS 3 dB"
                                 : "TRUE PEAK SAFETY ACTIVE",
                             juce::dontSendNotification);
    characterWarning.setColour(juce::Label::textColourId,
                               heldGainReduction > 3.0f ? warning : accent);
    release.setEnabled(! autoRelease.getToggleState());
}

void RemixSafeMasterAudioProcessorEditor::configureRotary(juce::Slider& slider,
                                                           const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 24);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, background.brighter(0.4f));
    slider.setColour(juce::Slider::textBoxTextColourId, text);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, background);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void RemixSafeMasterAudioProcessorEditor::configureMeterLabel(juce::Label& label)
{
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, text);
    label.setColour(juce::Label::backgroundColourId, background.withAlpha(0.8f));
}

juce::String RemixSafeMasterAudioProcessorEditor::formatDecibels(
    float value,
    const juce::String& suffix)
{
    return value <= -200.0f ? "-inf" + suffix : juce::String(value, 1) + suffix;
}
