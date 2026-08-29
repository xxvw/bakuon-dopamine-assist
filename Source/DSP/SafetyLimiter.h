#pragma once

#include "GainComputer.h"
#include "Metering.h"
#include "SmoothedValue.h"
#include "TruePeakDetector.h"

#include <array>
#include <vector>

namespace bakuon::dsp
{
struct LimiterParameters
{
    double inputTrimDb = 0.0;
    double ceilingDbTp = -1.0;
    double releaseMilliseconds = 100.0;
    bool autoRelease = true;
    int oversamplingFactor = OversamplingStage::normalFactor;
    bool bypass = false;
};

class SafetyLimiter
{
public:
    static constexpr double lookaheadMilliseconds = 1.5;
    static constexpr double guardMarginDecibels = 0.05;
    static constexpr double conformanceCalibrationDecibels = 0.02;
    static constexpr double parameterSmoothingMilliseconds = 10.0;

    void prepare(double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    void setParameters(const LimiterParameters& newParameters) noexcept;

    void beginBlock() noexcept;
    std::array<double, OversamplingStage::maxChannels> processFrame(
        const std::array<double, OversamplingStage::maxChannels>& input,
        int channels) noexcept;
    MeterValues endBlock() const noexcept;

    int getLatencySamples() const noexcept { return latencySamples; }
    double getCurrentGain() const noexcept { return gainComputer.getCurrentGain(); }

private:
    double sampleRate = 44100.0;
    int activeChannels = 2;
    int lookaheadSamples = 66;
    int latencySamples = 81;
    int delayWritePosition = 0;

    LimiterParameters parameters;
    LinearSmoothedValue inputTrimSmoother;
    LinearSmoothedValue ceilingSmoother;
    LinearSmoothedValue bypassSmoother;
    GainComputer gainComputer;
    TruePeakDetector inputTruePeakDetector;
    TruePeakDetector conformanceTruePeakDetector;
    TruePeakDetector outputTruePeakDetector;

    std::array<std::vector<double>, OversamplingStage::maxChannels> wetDelay;
    std::array<std::vector<double>, OversamplingStage::maxChannels> dryDelay;

    double blockInputSamplePeak = 0.0;
    double blockInputTruePeak = 0.0;
    double blockOutputSamplePeak = 0.0;
    double blockOutputTruePeak = 0.0;
    double blockPeakGainReduction = 0.0;
};
}
