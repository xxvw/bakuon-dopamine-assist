#pragma once

#include "OversamplingStage.h"

namespace bakuon::dsp
{
struct TruePeakFrame
{
    std::array<double, OversamplingStage::maxChannels> channelPeaks {};
    double linkedPeak = 0.0;
};

class TruePeakDetector
{
public:
    void reset() noexcept;
    void setOversamplingFactor(int newFactor) noexcept;
    int getOversamplingFactor() const noexcept { return oversamplingFactor; }

    TruePeakFrame processFrame(const std::array<double, OversamplingStage::maxChannels>& frame,
                               int channels) noexcept;

private:
    OversamplingStage oversampling;
    int oversamplingFactor = OversamplingStage::normalFactor;
};
}
