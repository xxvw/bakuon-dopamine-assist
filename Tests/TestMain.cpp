#include <juce_core/juce_core.h>

namespace
{
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
}

int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;
    for (int index = 0; index < runner.getNumResults(); ++index)
        failures += runner.getResult(index)->failures;

    return failures == 0 ? 0 : 1;
}
