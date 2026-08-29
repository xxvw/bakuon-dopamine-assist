#include "TruePeakDetector.h"

#include <algorithm>
#include <cmath>

namespace bakuon::dsp
{
void TruePeakDetector::reset() noexcept
{
    oversampling.reset();
}

void TruePeakDetector::setOversamplingFactor(int newFactor) noexcept
{
    if (newFactor == OversamplingStage::conformanceFactor)
        oversamplingFactor = OversamplingStage::conformanceFactor;
    else if (newFactor == OversamplingStage::highFactor)
        oversamplingFactor = OversamplingStage::highFactor;
    else
        oversamplingFactor = OversamplingStage::normalFactor;
}

TruePeakFrame TruePeakDetector::processFrame(
    const std::array<double, OversamplingStage::maxChannels>& frame,
    int channels) noexcept
{
    oversampling.pushFrame(frame, channels);

    TruePeakFrame result;
    const auto activeChannels = std::clamp(channels, 0, OversamplingStage::maxChannels);
    for (int channel = 0; channel < activeChannels; ++channel)
    {
        double peak = 0.0;
        for (int phase = 0; phase < oversamplingFactor; ++phase)
            peak = std::max(peak, std::abs(oversampling.interpolate(channel,
                                                                   phase,
                                                                   oversamplingFactor)));

        result.channelPeaks[static_cast<std::size_t>(channel)] = peak;
        result.linkedPeak = std::max(result.linkedPeak, peak);
    }

    return result;
}
}
