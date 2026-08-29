#include "Parameters.h"

namespace bakuon::parameters
{
namespace
{
juce::AudioParameterFloatAttributes decibelAttributes(const juce::String& suffix)
{
    return juce::AudioParameterFloatAttributes()
        .withLabel(suffix)
        .withStringFromValueFunction([](float value, int)
        {
            return juce::String(value, 1);
        })
        .withValueFromStringFunction([](const juce::String& text)
        {
            return text.getFloatValue();
        });
}
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ids::inputTrim, 1),
        "Input Trim",
        juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f),
        0.0f,
        decibelAttributes("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ids::ceiling, 1),
        "Ceiling",
        juce::NormalisableRange<float>(-3.0f, -0.1f, 0.1f),
        -1.0f,
        decibelAttributes("dBTP")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ids::release, 1),
        "Release",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ids::autoRelease, 1),
        "Auto Release",
        true));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ids::quality, 1),
        "Quality",
        juce::StringArray { "Normal (4x)", "High (8x)" },
        0));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ids::bypass, 1),
        "Bypass",
        false));

    return layout;
}

RawParameterPointers getRawParameterPointers(juce::AudioProcessorValueTreeState& state)
{
    return {
        state.getRawParameterValue(ids::inputTrim),
        state.getRawParameterValue(ids::ceiling),
        state.getRawParameterValue(ids::release),
        state.getRawParameterValue(ids::autoRelease),
        state.getRawParameterValue(ids::quality),
        state.getRawParameterValue(ids::bypass)
    };
}
}
