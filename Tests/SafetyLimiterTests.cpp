#include "DSP/DspUtilities.h"
#include "DSP/SafetyLimiter.h"
#include "ReferenceTruePeakMeter.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

namespace
{
using Frame = std::array<double, 2>;

struct ProcessResult
{
    std::vector<Frame> output;
    int latencySamples = 0;
    bakuon::dsp::MeterValues meters;
};

ProcessResult processSignal(const std::vector<Frame>& input,
                            double sampleRate,
                            const bakuon::dsp::LimiterParameters& parameters,
                            int channels = 2,
                            int blockSize = 257)
{
    bakuon::dsp::SafetyLimiter limiter;
    limiter.setParameters(parameters);
    limiter.prepare(sampleRate, blockSize, channels);

    ProcessResult result;
    result.latencySamples = limiter.getLatencySamples();
    result.output.reserve(input.size() + static_cast<std::size_t>(result.latencySamples + 96));

    int blockPosition = 0;
    limiter.beginBlock();
    const auto processFrame = [&](const Frame& frame)
    {
        result.output.push_back(limiter.processFrame(frame, channels));
        ++blockPosition;
        if (blockPosition == blockSize)
        {
            result.meters = limiter.endBlock();
            limiter.beginBlock();
            blockPosition = 0;
        }
    };

    for (const auto& frame : input)
        processFrame(frame);
    for (int flush = 0; flush < result.latencySamples + 96; ++flush)
        processFrame({});

    result.meters = limiter.endBlock();
    return result;
}

std::vector<Frame> makeSine(double sampleRate,
                            double frequency,
                            double amplitude,
                            int samples,
                            double phase = 0.0)
{
    std::vector<Frame> signal(static_cast<std::size_t>(samples));
    for (int sample = 0; sample < samples; ++sample)
    {
        const auto value = amplitude * std::sin(2.0 * std::numbers::pi * frequency
                                               * static_cast<double>(sample) / sampleRate
                                               + phase);
        signal[static_cast<std::size_t>(sample)] = { value, value };
    }
    return signal;
}

class SafetyLimiterTest final : public juce::UnitTest
{
public:
    SafetyLimiterTest() : UnitTest("Safety limiter") {}

    void runTest() override
    {
        testTransparency();
        testTruePeakCeiling();
        testFloatingPointHeadroomAndNumericSafety();
        testStereoLink();
        testInputTrimAndParameterSmoothing();
        testBypass();
        testBypassTransition();
        testSampleRatesAndReset();
    }

private:
    void testTransparency()
    {
        beginTest("Signal below threshold is delayed but unchanged");
        const auto input = makeSine(48000.0, 997.0, bakuon::dsp::decibelsToLinear(-6.0), 4096);
        const auto result = processSignal(input, 48000.0, {});

        double maximumDifference = 0.0;
        for (int sample = result.latencySamples;
             sample < static_cast<int>(input.size());
             ++sample)
        {
            const auto expected = input[static_cast<std::size_t>(sample - result.latencySamples)][0];
            maximumDifference = std::max(maximumDifference,
                                         std::abs(result.output[static_cast<std::size_t>(sample)][0]
                                                  - expected));
        }
        expect(maximumDifference < 1.0e-12,
               "Limiter must be transparent when no gain reduction is required");
    }

    void testTruePeakCeiling()
    {
        beginTest("Independent 16x reference stays within Ceiling +0.05 dBTP");
        const std::array<double, 4> sampleRates { 44100.0, 48000.0, 96000.0, 192000.0 };
        const auto allowed = bakuon::dsp::decibelsToLinear(-0.95);

        for (const auto sampleRate : sampleRates)
        {
            std::array<std::vector<Frame>, 4> signals;
            signals[0] = makeSine(sampleRate,
                                  sampleRate * 0.23,
                                  3.2,
                                  4096,
                                  0.37);

            signals[1].assign(4096, {});
            signals[1][32] = { 8.0, -8.0 };

            signals[2] = makeSine(sampleRate,
                                  sampleRate * 0.18,
                                  3.0,
                                  4096,
                                  0.19);
            for (auto& frame : signals[2])
                for (auto& sample : frame)
                    sample = std::clamp(sample, -1.6, 1.6);

            signals[3] = makeSine(sampleRate,
                                  sampleRate * 0.12,
                                  2.0,
                                  4096,
                                  0.51);
            for (auto& frame : signals[3])
                for (auto& sample : frame)
                    sample = sample >= 0.0 ? 2.0 : -2.0;

            for (std::size_t signalIndex = 0; signalIndex < signals.size(); ++signalIndex)
            {
                bakuon::dsp::LimiterParameters parameters;
                parameters.ceilingDbTp = -1.0;
                parameters.oversamplingFactor = signalIndex % 2 == 0 ? 4 : 8;
                const auto result = processSignal(signals[signalIndex],
                                                  sampleRate,
                                                  parameters,
                                                  2,
                                                  113);
                const auto measured = bakuon::test::ReferenceTruePeakMeter::measureLinear(
                    result.output,
                    2);

                expect(measured <= allowed,
                       "Fs=" + juce::String(sampleRate, 0)
                       + ", signal=" + juce::String(static_cast<int>(signalIndex))
                       + ", output="
                       + juce::String(bakuon::dsp::linearToDecibels(measured), 4)
                       + " dBTP; limit is -0.95 dBTP");
            }
        }
    }

    void testFloatingPointHeadroomAndNumericSafety()
    {
        beginTest("+18 dBFS, NaN and Inf remain finite");
        std::vector<Frame> input(2048, { bakuon::dsp::decibelsToLinear(18.0),
                                         -bakuon::dsp::decibelsToLinear(18.0) });
        input[11][0] = std::numeric_limits<double>::quiet_NaN();
        input[29][1] = std::numeric_limits<double>::infinity();
        input[47][0] = -std::numeric_limits<double>::infinity();

        const auto result = processSignal(input, 96000.0, {});
        for (const auto& frame : result.output)
            for (const auto sample : frame)
                expect(std::isfinite(sample));
    }

    void testStereoLink()
    {
        beginTest("Hot channel applies the same gain to both channels");
        constexpr auto sampleRate = 48000.0;
        std::vector<Frame> input(6000);
        for (int sample = 0; sample < static_cast<int>(input.size()); ++sample)
        {
            const auto wave = std::sin(2.0 * std::numbers::pi * 4000.0
                                       * static_cast<double>(sample) / sampleRate);
            input[static_cast<std::size_t>(sample)] = { 2.0 * wave, 0.2 * wave };
        }

        const auto result = processSignal(input, sampleRate, {});
        double maximumRatioError = 0.0;
        for (int sample = result.latencySamples + 500; sample < 5500; ++sample)
        {
            const auto left = result.output[static_cast<std::size_t>(sample)][0];
            const auto right = result.output[static_cast<std::size_t>(sample)][1];
            if (std::abs(left) > 1.0e-6)
                maximumRatioError = std::max(maximumRatioError,
                                             std::abs(right / left - 0.1));
        }
        expect(maximumRatioError < 1.0e-10);
    }

    void testBypass()
    {
        beginTest("Bypass is latency aligned and does not clamp floating input");
        std::vector<Frame> input(1024, { 2.0, -2.0 });
        bakuon::dsp::LimiterParameters parameters;
        parameters.bypass = true;
        const auto result = processSignal(input, 48000.0, parameters);

        const auto output = result.output[static_cast<std::size_t>(result.latencySamples + 100)];
        expectWithinAbsoluteError(output[0], 2.0, 1.0e-12);
        expectWithinAbsoluteError(output[1], -2.0, 1.0e-12);
    }

    void testInputTrimAndParameterSmoothing()
    {
        beginTest("Input Trim is applied and automation is smoothed");
        constexpr auto sampleRate = 48000.0;
        bakuon::dsp::SafetyLimiter limiter;
        bakuon::dsp::LimiterParameters parameters;
        parameters.inputTrimDb = -6.0;
        limiter.setParameters(parameters);
        limiter.prepare(sampleRate, 127, 1);
        limiter.beginBlock();

        std::vector<double> output;
        output.reserve(2400);
        for (int sample = 0; sample < 1200; ++sample)
            output.push_back(limiter.processFrame({ 0.1, 0.0 }, 1)[0]);

        const auto expectedTrimmed = 0.1 * bakuon::dsp::decibelsToLinear(-6.0);
        expectWithinAbsoluteError(output[static_cast<std::size_t>(limiter.getLatencySamples() + 700)],
                                  expectedTrimmed,
                                  1.0e-12);

        parameters.inputTrimDb = 6.0;
        limiter.setParameters(parameters);
        for (int sample = 0; sample < 1200; ++sample)
            output.push_back(limiter.processFrame({ 0.1, 0.0 }, 1)[0]);

        double largestAdjacentStep = 0.0;
        const auto transitionStart = static_cast<std::size_t>(1200 + limiter.getLatencySamples());
        const auto transitionEnd = std::min(output.size(), transitionStart + 700);
        for (std::size_t sample = transitionStart; sample < transitionEnd; ++sample)
            largestAdjacentStep = std::max(largestAdjacentStep,
                                           std::abs(output[sample] - output[sample - 1]));

        expect(largestAdjacentStep < 0.001,
               "10 ms Input Trim smoothing must avoid a discontinuity");
        expectWithinAbsoluteError(output.back(),
                                  0.1 * bakuon::dsp::decibelsToLinear(6.0),
                                  1.0e-9);
    }

    void testBypassTransition()
    {
        beginTest("Bypass transition is latency aligned and crossfaded");
        constexpr auto sampleRate = 48000.0;
        bakuon::dsp::SafetyLimiter limiter;
        bakuon::dsp::LimiterParameters parameters;
        limiter.setParameters(parameters);
        limiter.prepare(sampleRate, 113, 1);
        limiter.beginBlock();

        std::vector<double> output;
        output.reserve(3000);
        for (int sample = 0; sample < 1500; ++sample)
            output.push_back(limiter.processFrame({ 2.0, 0.0 }, 1)[0]);

        parameters.bypass = true;
        limiter.setParameters(parameters);
        for (int sample = 0; sample < 1500; ++sample)
            output.push_back(limiter.processFrame({ 2.0, 0.0 }, 1)[0]);

        double largestTransitionStep = 0.0;
        for (std::size_t sample = 1400; sample < 2100; ++sample)
            largestTransitionStep = std::max(largestTransitionStep,
                                             std::abs(output[sample] - output[sample - 1]));

        expect(largestTransitionStep < 0.01,
               "10 ms bypass crossfade must avoid a large discontinuity");
        expectWithinAbsoluteError(output.back(), 2.0, 1.0e-12);
    }

    void testSampleRatesAndReset()
    {
        beginTest("Supported sample rates and irregular block sizes are stable");
        const std::array<double, 6> sampleRates { 44100.0, 48000.0, 88200.0,
                                                  96000.0, 176400.0, 192000.0 };
        const std::array<int, 7> blockSizes { 1, 3, 31, 127, 511, 1023, 8192 };

        for (const auto sampleRate : sampleRates)
        {
            for (const auto blockSize : blockSizes)
            {
                bakuon::dsp::SafetyLimiter limiter;
                limiter.prepare(sampleRate, blockSize, 1);
                limiter.beginBlock();
                for (int sample = 0; sample < 64; ++sample)
                {
                    const auto output = limiter.processFrame({ sample == 0 ? 1.5 : 0.0, 0.0 }, 1);
                    expect(std::isfinite(output[0]));
                }
                limiter.reset();
                limiter.beginBlock();
                for (int sample = 0; sample < limiter.getLatencySamples() + 8; ++sample)
                    expectWithinAbsoluteError(limiter.processFrame({}, 1)[0], 0.0, 1.0e-15);
            }
        }
    }
};

SafetyLimiterTest safetyLimiterTest;
}
