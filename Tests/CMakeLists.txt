juce_add_console_app(RemixSafeMasterTests PRODUCT_NAME "Remix Safe Master Tests")

target_sources(RemixSafeMasterTests PRIVATE
    TestMain.cpp
    ReferenceTruePeakMeter.cpp
    ReferenceTruePeakMeter.h
    SafetyLimiterTests.cpp
    TruePeakDetectorTests.cpp
    ../Source/DSP/GainComputer.cpp
    ../Source/DSP/GainComputer.h
    ../Source/DSP/Metering.h
    ../Source/DSP/OversamplingStage.cpp
    ../Source/DSP/OversamplingStage.h
    ../Source/DSP/SafetyLimiter.cpp
    ../Source/DSP/SafetyLimiter.h
    ../Source/DSP/SmoothedValue.h
    ../Source/DSP/TruePeakDetector.cpp
    ../Source/DSP/TruePeakDetector.h
    ../Source/Parameters/Parameters.cpp
    ../Source/Parameters/Parameters.h)

target_include_directories(RemixSafeMasterTests PRIVATE ../Source)

target_compile_definitions(RemixSafeMasterTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(RemixSafeMasterTests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME RemixSafeMasterTests COMMAND RemixSafeMasterTests)

juce_add_console_app(RemixSafeMasterUISnapshot
    PRODUCT_NAME "Remix Safe Master UI Snapshot")
juce_generate_juce_header(RemixSafeMasterUISnapshot)

target_sources(RemixSafeMasterUISnapshot PRIVATE UiSnapshot.cpp)
target_include_directories(RemixSafeMasterUISnapshot PRIVATE ../Source)
target_link_libraries(RemixSafeMasterUISnapshot PRIVATE
    RemixSafeMaster
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
    target_compile_options(RemixSafeMasterUISnapshot PRIVATE -Wno-macro-redefined)
endif()

add_test(NAME RemixSafeMasterUISnapshot COMMAND RemixSafeMasterUISnapshot)
