#include "ReferenceTruePeakMeter.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace bakuon::test
{
namespace
{
double sinc(double value)
{
    return std::abs(value) < 1.0e-14
        ? 1.0
        : std::sin(std::numbers::pi * value) / (std::numbers::pi * value);
}

double blackman(double normalisedDistance)
{
    if (std::abs(normalisedDistance) > 1.0)
        return 0.0;
    return 0.42 + 0.5 * std::cos(std::numbers::pi * normalisedDistance)
                + 0.08 * std::cos(2.0 * std::numbers::pi * normalisedDistance);
}
}

double ReferenceTruePeakMeter::measureLinear(
    const std::vector<std::array<double, 2>>& signal,
    int channels)
{
    constexpr int oversamplingFactor = 16;
    constexpr int radius = 32;
    double maximum = 0.0;

    for (int channel = 0; channel < std::clamp(channels, 1, 2); ++channel)
    {
        for (int centre = 0; centre < static_cast<int>(signal.size()); ++centre)
        {
            for (int phase = 0; phase < oversamplingFactor; ++phase)
            {
                const auto fraction = static_cast<double>(phase)
                                    / static_cast<double>(oversamplingFactor);
                double value = 0.0;
                double coefficientSum = 0.0;

                for (int offset = -radius + 1; offset <= radius; ++offset)
                {
                    const auto distance = fraction - static_cast<double>(offset);
                    const auto coefficient = sinc(distance)
                                           * blackman(distance / static_cast<double>(radius));
                    coefficientSum += coefficient;

                    const auto sampleIndex = centre + offset;
                    if (sampleIndex >= 0 && sampleIndex < static_cast<int>(signal.size()))
                        value += signal[static_cast<std::size_t>(sampleIndex)]
                                       [static_cast<std::size_t>(channel)] * coefficient;
                }

                if (std::abs(coefficientSum) > 1.0e-14)
                    value /= coefficientSum;
                maximum = std::max(maximum, std::abs(value));
            }
        }
    }

    return maximum;
}
}
