#include "OversamplingStage.h"
#include "DspUtilities.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace bakuon::dsp
{
const std::array<std::array<double, OversamplingStage::normalTaps>,
                 OversamplingStage::normalFactor> OversamplingStage::annexCoefficients {{
    {{ 0.0017089843750,  0.0109863281250, -0.0196533203125,  0.0332031250000,
      -0.0594482421875,  0.1373291015625,  0.9721679687500, -0.1022949218750,
       0.0476074218750, -0.0266113281250,  0.0148925781250, -0.0083007812500 }},
    {{-0.0291748046875,  0.0292968750000, -0.0517578125000,  0.0891113281250,
      -0.1665039062500,  0.4650878906250,  0.7797851562500, -0.2003173828125,
       0.1015625000000, -0.0582275390625,  0.0330810546875, -0.0189208984375 }},
    {{-0.0189208984375,  0.0330810546875, -0.0582275390625,  0.1015625000000,
      -0.2003173828125,  0.7797851562500,  0.4650878906250, -0.1665039062500,
       0.0891113281250, -0.0517578125000,  0.0292968750000, -0.0291748046875 }},
    {{-0.0083007812500,  0.0148925781250, -0.0266113281250,  0.0476074218750,
      -0.1022949218750,  0.9721679687500,  0.1373291015625, -0.0594482421875,
       0.0332031250000, -0.0196533203125,  0.0109863281250,  0.0017089843750 }}
}};

OversamplingStage::OversamplingStage()
{
    buildHighQualityCoefficients();
    reset();
}

void OversamplingStage::reset() noexcept
{
    for (auto& channel : history)
        channel.fill(0.0);
    writePosition = 0;
}

void OversamplingStage::pushFrame(const std::array<double, maxChannels>& frame,
                                  int channels) noexcept
{
    for (int channel = 0; channel < maxChannels; ++channel)
        history[static_cast<std::size_t>(channel)][static_cast<std::size_t>(writePosition)] =
            channel < channels ? sanitiseSample(frame[static_cast<std::size_t>(channel)]) : 0.0;

    writePosition = (writePosition + 1) % highTaps;
}

double OversamplingStage::interpolate(int channel, int phase, int factor) const noexcept
{
    if (channel < 0 || channel >= maxChannels)
        return 0.0;

    const auto useAnnex = factor == normalFactor;
    const auto phases = factor == conformanceFactor
        ? conformanceFactor
        : (factor == highFactor ? highFactor : normalFactor);
    const auto taps = useAnnex ? normalTaps : highTaps;
    const auto safePhase = std::clamp(phase, 0, phases - 1);

    double value = 0.0;
    for (int tap = 0; tap < taps; ++tap)
    {
        const auto coefficientPhase = factor == conformanceFactor
            ? safePhase
            : (factor == highFactor ? safePhase * 2 : safePhase * 4);
        const auto coefficient = ! useAnnex
            ? highQualityCoefficients[static_cast<std::size_t>(coefficientPhase)]
                                     [static_cast<std::size_t>(tap)]
            : annexCoefficients[static_cast<std::size_t>(safePhase)][static_cast<std::size_t>(tap)];
        value += coefficient * historySample(channel, tap);
    }

    return finiteOrZero(value);
}

void OversamplingStage::buildHighQualityCoefficients() noexcept
{
    constexpr auto centre = 31.0;
    constexpr auto support = 32.0;

    for (int phase = 0; phase < conformanceFactor; ++phase)
    {
        const auto fraction = static_cast<double>(phase)
                            / static_cast<double>(conformanceFactor);
        double sum = 0.0;

        for (int tap = 0; tap < highTaps; ++tap)
        {
            const auto distance = static_cast<double>(tap) - centre + fraction;
            const auto sinc = std::abs(distance) < 1.0e-12
                ? 1.0
                : std::sin(std::numbers::pi * distance) / (std::numbers::pi * distance);

            const auto normalised = distance / support;
            const auto window = std::abs(normalised) <= 1.0
                ? 0.42 + 0.5 * std::cos(std::numbers::pi * normalised)
                       + 0.08 * std::cos(2.0 * std::numbers::pi * normalised)
                : 0.0;

            const auto coefficient = sinc * window;
            highQualityCoefficients[static_cast<std::size_t>(phase)][static_cast<std::size_t>(tap)] =
                coefficient;
            sum += coefficient;
        }

        if (std::abs(sum) > meterEpsilon)
            for (auto& coefficient : highQualityCoefficients[static_cast<std::size_t>(phase)])
                coefficient /= sum;
    }
}

double OversamplingStage::historySample(int channel, int samplesBack) const noexcept
{
    auto position = writePosition - 1 - samplesBack;
    while (position < 0)
        position += highTaps;
    return history[static_cast<std::size_t>(channel)][static_cast<std::size_t>(position)];
}
}
