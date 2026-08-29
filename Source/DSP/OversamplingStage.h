#pragma once

#include <array>

namespace bakuon::dsp
{
class OversamplingStage
{
public:
    static constexpr int normalFactor = 4;
    static constexpr int highFactor = 8;
    static constexpr int conformanceFactor = 16;
    static constexpr int maxChannels = 2;
    static constexpr int normalTaps = 12;
    static constexpr int highTaps = 64;
    static constexpr int maximumLatencySamples = 31;

    OversamplingStage();

    void reset() noexcept;
    void pushFrame(const std::array<double, maxChannels>& frame, int channels) noexcept;
    double interpolate(int channel, int phase, int factor) const noexcept;

private:
    void buildHighQualityCoefficients() noexcept;
    double historySample(int channel, int samplesBack) const noexcept;

    static const std::array<std::array<double, normalTaps>, normalFactor> annexCoefficients;

    std::array<std::array<double, highTaps>, conformanceFactor> highQualityCoefficients {};
    std::array<std::array<double, highTaps>, maxChannels> history {};
    int writePosition = 0;
};
}
