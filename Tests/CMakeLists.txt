juce_add_console_app(RemixSafeMasterTests PRODUCT_NAME "Remix Safe Master Tests")

target_sources(RemixSafeMasterTests PRIVATE
    TestMain.cpp
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
