#pragma once

#include <atomic>

namespace bakuon::dsp
{
struct MeterValues
{
    float inputSamplePeakDb = -240.0f;
    float inputTruePeakDb = -240.0f;
    float outputSamplePeakDb = -240.0f;
    float outputTruePeakDb = -240.0f;
    float currentGainReductionDb = 0.0f;
    float peakGainReductionDb = 0.0f;
};

class Metering
{
public:
    void publish(const MeterValues& values) noexcept
    {
        inputSamplePeakDb.store(values.inputSamplePeakDb, std::memory_order_relaxed);
        inputTruePeakDb.store(values.inputTruePeakDb, std::memory_order_relaxed);
        outputSamplePeakDb.store(values.outputSamplePeakDb, std::memory_order_relaxed);
        outputTruePeakDb.store(values.outputTruePeakDb, std::memory_order_relaxed);
        currentGainReductionDb.store(values.currentGainReductionDb, std::memory_order_relaxed);
        peakGainReductionDb.store(values.peakGainReductionDb, std::memory_order_relaxed);
    }

    MeterValues read() const noexcept
    {
        return {
            inputSamplePeakDb.load(std::memory_order_relaxed),
            inputTruePeakDb.load(std::memory_order_relaxed),
            outputSamplePeakDb.load(std::memory_order_relaxed),
            outputTruePeakDb.load(std::memory_order_relaxed),
            currentGainReductionDb.load(std::memory_order_relaxed),
            peakGainReductionDb.load(std::memory_order_relaxed)
        };
    }

private:
    std::atomic<float> inputSamplePeakDb { -240.0f };
    std::atomic<float> inputTruePeakDb { -240.0f };
    std::atomic<float> outputSamplePeakDb { -240.0f };
    std::atomic<float> outputTruePeakDb { -240.0f };
    std::atomic<float> currentGainReductionDb { 0.0f };
    std::atomic<float> peakGainReductionDb { 0.0f };
};
}
