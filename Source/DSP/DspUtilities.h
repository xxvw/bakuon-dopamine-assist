#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace bakuon::dsp
{
inline constexpr double meterEpsilon = 1.0e-12;

inline double decibelsToLinear(double decibels) noexcept
{
    return std::pow(10.0, decibels / 20.0);
}

inline double linearToDecibels(double linear) noexcept
{
    return 20.0 * std::log10(std::max(std::abs(linear), meterEpsilon));
}

inline double sanitiseSample(double sample) noexcept
{
    if (! std::isfinite(sample))
        return 0.0;

    if (std::abs(sample) < std::numeric_limits<double>::min())
        return 0.0;

    return sample;
}

inline double finiteOrZero(double value) noexcept
{
    return std::isfinite(value) ? value : 0.0;
}
}
