#pragma once

namespace bakuon::dsp
{
class GainComputer
{
public:
    void prepare(double sampleRate, int holdSamples) noexcept;
    void reset() noexcept;

    double process(double requiredGain,
                   double fixedReleaseMilliseconds,
                   bool useAutoRelease) noexcept;

    double getCurrentGain() const noexcept { return currentGain; }
    double getGainReductionDecibels() const noexcept;

private:
    double calculateReleaseMilliseconds(double fixedReleaseMilliseconds,
                                        bool useAutoRelease) const noexcept;

    double currentSampleRate = 44100.0;
    double currentGain = 1.0;
    double heldGain = 1.0;
    int holdDurationSamples = 1;
    int holdRemainingSamples = 0;
    int sustainedReductionSamples = 0;
};
}
