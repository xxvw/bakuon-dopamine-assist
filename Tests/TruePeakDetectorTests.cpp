#include "DSP/DspUtilities.h"
#include "DSP/TruePeakDetector.h"

#include <juce_core/juce_core.h>

#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace
{
class TruePeakDetectorTest final : public juce::UnitTest
{
public:
    TruePeakDetectorTest() : UnitTest("True Peak detector") {}

    void runTest() override
    {
        testSilence();
        testInterSamplePeak();
        testHighQualityMode();
        testNonFiniteInput();
    }

private:
    void testSilence()
    {
        beginTest("Silence remains zero");
        bakuon::dsp::TruePeakDetector detector;
        std::array<double, 2> frame {};

        for (int sample = 0; sample < 128; ++sample)
            expectWithinAbsoluteError(detector.processFrame(frame, 2).linkedPeak, 0.0, 1.0e-15);
    }

    void testInterSamplePeak()
    {
        beginTest("Annex 2 detector distinguishes sample and true peak");
        bakuon::dsp::TruePeakDetector detector;
        detector.setOversamplingFactor(4);

        double samplePeak = 0.0;
        double truePeak = 0.0;
        for (int sample = 0; sample < 256; ++sample)
        {
            const auto value = std::sin(2.0 * std::numbers::pi * 0.25
                                        * static_cast<double>(sample)
                                        + std::numbers::pi / 4.0);
            samplePeak = std::max(samplePeak, std::abs(value));
            truePeak = std::max(truePeak,
                                detector.processFrame({ value, value }, 2).linkedPeak);
        }

        expect(samplePeak < 0.72, "The test signal must have low sample peak");
        expect(truePeak > samplePeak + 0.20,
               "4x reconstruction must expose the inter-sample peak");
        expectWithinAbsoluteError(truePeak, 1.0, 0.06);
    }

    void testHighQualityMode()
    {
        beginTest("High mode performs 8x interpolation");
        bakuon::dsp::TruePeakDetector detector;
        detector.setOversamplingFactor(8);

        double truePeak = 0.0;
        for (int sample = 0; sample < 256; ++sample)
        {
            const auto value = std::sin(2.0 * std::numbers::pi * 0.25
                                        * static_cast<double>(sample)
                                        + std::numbers::pi / 4.0);
            truePeak = std::max(truePeak,
                                detector.processFrame({ value, value }, 2).linkedPeak);
        }

        expectWithinAbsoluteError(truePeak, 1.0, 0.03);
    }

    void testNonFiniteInput()
    {
        beginTest("NaN and Inf do not propagate");
        bakuon::dsp::TruePeakDetector detector;
        const auto result = detector.processFrame(
            { std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::infinity() },
            2);
        expect(std::isfinite(result.linkedPeak));
        expectWithinAbsoluteError(result.linkedPeak, 0.0, 1.0e-15);
    }
};

TruePeakDetectorTest truePeakDetectorTest;
}
