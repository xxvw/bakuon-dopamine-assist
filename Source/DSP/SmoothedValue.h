#pragma once

#include <algorithm>
#include <cmath>

namespace bakuon::dsp
{
class LinearSmoothedValue
{
public:
    void prepare(double newSampleRate, double rampMilliseconds) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        rampSamples = std::max(1, static_cast<int>(std::round(
            sampleRate * rampMilliseconds * 0.001)));
    }

    void reset(double value) noexcept
    {
        current = value;
        target = value;
        step = 0.0;
        remaining = 0;
    }

    void setTarget(double value) noexcept
    {
        if (std::abs(value - target) <= 1.0e-15)
            return;

        target = value;
        remaining = rampSamples;
        step = (target - current) / static_cast<double>(remaining);
    }

    double getNextValue() noexcept
    {
        if (remaining <= 0)
            return current;

        current += step;
        --remaining;
        if (remaining == 0)
            current = target;
        return current;
    }

    double getCurrentValue() const noexcept { return current; }

private:
    double sampleRate = 44100.0;
    double current = 0.0;
    double target = 0.0;
    double step = 0.0;
    int rampSamples = 1;
    int remaining = 0;
};
}
