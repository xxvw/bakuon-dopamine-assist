#include "GainComputer.h"
#include "DspUtilities.h"

#include <algorithm>
#include <cmath>

namespace bakuon::dsp
{
void GainComputer::prepare(double sampleRate, int holdSamples) noexcept
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    holdDurationSamples = std::max(1, holdSamples);
    reset();
}

void GainComputer::reset() noexcept
{
    currentGain = 1.0;
    heldGain = 1.0;
    holdRemainingSamples = 0;
    sustainedReductionSamples = 0;
}

double GainComputer::process(double requiredGain,
                             double fixedReleaseMilliseconds,
                             bool useAutoRelease) noexcept
{
    requiredGain = std::clamp(finiteOrZero(requiredGain), 0.0, 1.0);

    if (requiredGain < 1.0)
    {
        heldGain = std::min(heldGain, requiredGain);
        holdRemainingSamples = holdDurationSamples;
        ++sustainedReductionSamples;
    }
    else if (holdRemainingSamples > 0)
    {
        --holdRemainingSamples;
    }

    const auto target = holdRemainingSamples > 0 ? heldGain : 1.0;
    if (holdRemainingSamples == 0)
        heldGain = 1.0;

    if (target < currentGain)
    {
        constexpr auto attackTimeSeconds = 0.00015;
        const auto coefficient = std::exp(-1.0 / (attackTimeSeconds * currentSampleRate));
        currentGain = target + coefficient * (currentGain - target);
    }
    else
    {
        const auto releaseMilliseconds = calculateReleaseMilliseconds(fixedReleaseMilliseconds,
                                                                      useAutoRelease);
        const auto releaseSeconds = std::max(0.001, releaseMilliseconds * 0.001);
        const auto coefficient = std::exp(-1.0 / (releaseSeconds * currentSampleRate));
        currentGain = target + coefficient * (currentGain - target);

        if (currentGain > 0.999999)
        {
            currentGain = 1.0;
            sustainedReductionSamples = 0;
        }
    }

    currentGain = std::clamp(finiteOrZero(currentGain), 0.0, 1.0);
    return currentGain;
}

double GainComputer::getGainReductionDecibels() const noexcept
{
    return std::max(0.0, -linearToDecibels(currentGain));
}

double GainComputer::calculateReleaseMilliseconds(double fixedReleaseMilliseconds,
                                                  bool useAutoRelease) const noexcept
{
    if (! useAutoRelease)
        return std::clamp(fixedReleaseMilliseconds, 20.0, 500.0);

    const auto gainReduction = getGainReductionDecibels();
    const auto depthContribution = std::clamp(gainReduction / 12.0, 0.0, 1.0) * 120.0;
    const auto sustainedSeconds = static_cast<double>(sustainedReductionSamples)
                                / currentSampleRate;
    const auto durationContribution = std::clamp(sustainedSeconds, 0.0, 1.0) * 300.0;
    return std::clamp(60.0 + depthContribution + durationContribution, 50.0, 500.0);
}
}
