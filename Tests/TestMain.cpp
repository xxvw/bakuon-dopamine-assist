#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "Parameters/Parameters.h"

namespace
{
class DummyAudioProcessor final : public juce::AudioProcessor
{
public:
    DummyAudioProcessor() : AudioProcessor(BusesProperties()) {}

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "Parameter test host"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

class ProjectSmokeTest final : public juce::UnitTest
{
public:
    ProjectSmokeTest() : UnitTest("Project smoke test") {}

    void runTest() override
    {
        beginTest("JUCE test runner is operational");
        expect(true);
    }
};

ProjectSmokeTest projectSmokeTest;

class ParameterLayoutTest final : public juce::UnitTest
{
public:
    ParameterLayoutTest() : UnitTest("Parameter layout") {}

    void runTest() override
    {
        DummyAudioProcessor processor;
        juce::AudioProcessorValueTreeState state(
            processor,
            nullptr,
            bakuon::parameters::stateTreeType,
            bakuon::parameters::createParameterLayout());

        const auto findParameter = [&state](const juce::String& id)
        {
            return state.getParameter(id);
        };

        beginTest("Required parameters exist");
        expectEquals(processor.getParameters().size(), 6);

        const auto expectDefault = [this, &findParameter](const juce::String& id,
                                                          float expected)
        {
            const auto* parameter = findParameter(id);
            expect(parameter != nullptr, id + " is missing");
            if (parameter != nullptr)
                expectWithinAbsoluteError(parameter->convertFrom0to1(parameter->getDefaultValue()),
                                          expected,
                                          0.0001f);
        };

        beginTest("DOCX defaults are preserved");
        expectDefault(bakuon::parameters::ids::inputTrim, 0.0f);
        expectDefault(bakuon::parameters::ids::ceiling, -1.0f);
        expectDefault(bakuon::parameters::ids::autoRelease, 1.0f);
        expectDefault(bakuon::parameters::ids::quality, 0.0f);
        expectDefault(bakuon::parameters::ids::bypass, 0.0f);
    }
};

ParameterLayoutTest parameterLayoutTest;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;
    for (int index = 0; index < runner.getNumResults(); ++index)
        failures += runner.getResult(index)->failures;

    return failures == 0 ? 0 : 1;
}
