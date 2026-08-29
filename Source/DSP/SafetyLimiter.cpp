#include "SafetyLimiter.h"
#include "DspUtilities.h"

#include <algorithm>
#include <cmath>

namespace bakuon::dsp
{
void SafetyLimiter::prepare(double newSampleRate, int maximumBlockSize, int channels)
{
    static_cast<void>(maximumBlockSize);
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    activeChannels = std::clamp(channels, 1, OversamplingStage::maxChannels);
    lookaheadSamples = std::max(1, static_cast<int>(std::round(
        sampleRate * lookaheadMilliseconds * 0.001)));
    latencySamples = lookaheadSamples + OversamplingStage::maximumLatencySamples;

    for (auto& channel : wetDelay)
        channel.assign(static_cast<std::size_t>(latencySamples), 0.0);
    for (auto& channel : dryDelay)
        channel.assign(static_cast<std::size_t>(latencySamples), 0.0);

    inputTrimSmoother.prepare(sampleRate, parameterSmoothingMilliseconds);
    ceilingSmoother.prepare(sampleRate, parameterSmoothingMilliseconds);
    bypassSmoother.prepare(sampleRate, parameterSmoothingMilliseconds);
    inputTrimSmoother.reset(decibelsToLinear(parameters.inputTrimDb));
    ceilingSmoother.reset(decibelsToLinear(parameters.ceilingDbTp
                                           - guardMarginDecibels
                                           - conformanceCalibrationDecibels));
    bypassSmoother.reset(parameters.bypass ? 1.0 : 0.0);

    gainComputer.prepare(sampleRate, latencySamples);
    inputTruePeakDetector.setOversamplingFactor(parameters.oversamplingFactor);
    conformanceTruePeakDetector.setOversamplingFactor(OversamplingStage::conformanceFactor);
    outputTruePeakDetector.setOversamplingFactor(parameters.oversamplingFactor);
    reset();
}

void SafetyLimiter::reset() noexcept
{
    for (auto& channel : wetDelay)
        std::fill(channel.begin(), channel.end(), 0.0);
    for (auto& channel : dryDelay)
        std::fill(channel.begin(), channel.end(), 0.0);

    delayWritePosition = 0;
    gainComputer.reset();
    inputTruePeakDetector.reset();
    conformanceTruePeakDetector.reset();
    outputTruePeakDetector.reset();
    beginBlock();
}

void SafetyLimiter::setParameters(const LimiterParameters& newParameters) noexcept
{
    parameters = newParameters;
    parameters.inputTrimDb = std::clamp(parameters.inputTrimDb, -24.0, 6.0);
    parameters.ceilingDbTp = std::clamp(parameters.ceilingDbTp, -3.0, -0.1);
    parameters.releaseMilliseconds = std::clamp(parameters.releaseMilliseconds, 20.0, 500.0);
    parameters.oversamplingFactor = parameters.oversamplingFactor == OversamplingStage::highFactor
        ? OversamplingStage::highFactor
        : OversamplingStage::normalFactor;

    inputTrimSmoother.setTarget(decibelsToLinear(parameters.inputTrimDb));
    ceilingSmoother.setTarget(decibelsToLinear(parameters.ceilingDbTp
                                               - guardMarginDecibels
                                               - conformanceCalibrationDecibels));
    bypassSmoother.setTarget(parameters.bypass ? 1.0 : 0.0);
    inputTruePeakDetector.setOversamplingFactor(parameters.oversamplingFactor);
    outputTruePeakDetector.setOversamplingFactor(parameters.oversamplingFactor);
}

void SafetyLimiter::beginBlock() noexcept
{
    blockInputSamplePeak = 0.0;
    blockInputTruePeak = 0.0;
    blockOutputSamplePeak = 0.0;
    blockOutputTruePeak = 0.0;
    blockPeakGainReduction = 0.0;
}

std::array<double, OversamplingStage::maxChannels> SafetyLimiter::processFrame(
    const std::array<double, OversamplingStage::maxChannels>& input,
    int channels) noexcept
{
    const auto numChannels = std::clamp(channels, 1, activeChannels);
    const auto trimGain = inputTrimSmoother.getNextValue();
    const auto effectiveCeiling = ceilingSmoother.getNextValue();
    const auto bypassMix = std::clamp(bypassSmoother.getNextValue(), 0.0, 1.0);

    std::array<double, OversamplingStage::maxChannels> sanitised {};
    std::array<double, OversamplingStage::maxChannels> trimmed {};
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto index = static_cast<std::size_t>(channel);
        sanitised[index] = sanitiseSample(input[index]);
        trimmed[index] = finiteOrZero(sanitised[index] * trimGain);
        blockInputSamplePeak = std::max(blockInputSamplePeak, std::abs(trimmed[index]));
    }

    const auto detectedInput = inputTruePeakDetector.processFrame(trimmed, numChannels);
    blockInputTruePeak = std::max(blockInputTruePeak, detectedInput.linkedPeak);
    const auto conformancePeak = conformanceTruePeakDetector.processFrame(trimmed,
                                                                          numChannels).linkedPeak;
    const auto safetyPeak = std::max(detectedInput.linkedPeak, conformancePeak);
    const auto requiredGain = safetyPeak > meterEpsilon
        ? std::min(1.0, effectiveCeiling / (safetyPeak + meterEpsilon))
        : 1.0;
    const auto gain = gainComputer.process(requiredGain,
                                           parameters.releaseMilliseconds,
                                           parameters.autoRelease);
    blockPeakGainReduction = std::max(blockPeakGainReduction,
                                      gainComputer.getGainReductionDecibels());

    std::array<double, OversamplingStage::maxChannels> output {};
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto index = static_cast<std::size_t>(channel);
        const auto delayedWet = wetDelay[index][static_cast<std::size_t>(delayWritePosition)];
        const auto delayedDry = dryDelay[index][static_cast<std::size_t>(delayWritePosition)];
        wetDelay[index][static_cast<std::size_t>(delayWritePosition)] = trimmed[index];
        dryDelay[index][static_cast<std::size_t>(delayWritePosition)] = sanitised[index];

        const auto limited = finiteOrZero(delayedWet * gain);
        output[index] = finiteOrZero(limited + (delayedDry - limited) * bypassMix);
        blockOutputSamplePeak = std::max(blockOutputSamplePeak, std::abs(output[index]));
    }

    delayWritePosition = (delayWritePosition + 1) % latencySamples;
    const auto detectedOutput = outputTruePeakDetector.processFrame(output, numChannels);
    blockOutputTruePeak = std::max(blockOutputTruePeak, detectedOutput.linkedPeak);
    return output;
}

MeterValues SafetyLimiter::endBlock() const noexcept
{
    return {
        static_cast<float>(linearToDecibels(blockInputSamplePeak)),
        static_cast<float>(linearToDecibels(blockInputTruePeak)),
        static_cast<float>(linearToDecibels(blockOutputSamplePeak)),
        static_cast<float>(linearToDecibels(blockOutputTruePeak)),
        static_cast<float>(gainComputer.getGainReductionDecibels()),
        static_cast<float>(blockPeakGainReduction)
    };
}
}
