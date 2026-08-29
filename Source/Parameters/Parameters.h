#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace bakuon::parameters
{
inline constexpr auto stateTreeType = "RemixSafeMasterState";
inline constexpr int stateSchemaVersion = 1;

namespace ids
{
inline constexpr auto inputTrim = "inputTrim";
inline constexpr auto ceiling = "ceiling";
inline constexpr auto release = "release";
inline constexpr auto autoRelease = "autoRelease";
inline constexpr auto quality = "quality";
inline constexpr auto bypass = "bypass";
}

struct RawParameterPointers
{
    std::atomic<float>* inputTrim = nullptr;
    std::atomic<float>* ceiling = nullptr;
    std::atomic<float>* release = nullptr;
    std::atomic<float>* autoRelease = nullptr;
    std::atomic<float>* quality = nullptr;
    std::atomic<float>* bypass = nullptr;
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
RawParameterPointers getRawParameterPointers(juce::AudioProcessorValueTreeState& state);
}
