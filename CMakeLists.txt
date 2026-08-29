cmake_minimum_required(VERSION 3.22)

project(bakuon_dopamine_assist VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BAKUON_BUILD_STANDALONE "Build the QA standalone application" ON)
option(BAKUON_BUILD_TESTS "Build DSP and plug-in unit tests" ON)

add_subdirectory(third_party/JUCE)

set(BAKUON_PLUGIN_FORMATS AU VST3)
if(BAKUON_BUILD_STANDALONE)
    list(APPEND BAKUON_PLUGIN_FORMATS Standalone)
endif()

juce_add_plugin(RemixSafeMaster
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "bakuon-dopamine-assist"
    BUNDLE_ID audio.bakuon.remix-safe-master
    PLUGIN_MANUFACTURER_CODE Baku
    PLUGIN_CODE RsMp
    PRODUCT_NAME "Remix Safe Master"
    DESCRIPTION "True Peak safety processor for intentionally distorted masters"
    FORMATS ${BAKUON_PLUGIN_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect)

juce_generate_juce_header(RemixSafeMaster)

target_sources(RemixSafeMaster PRIVATE
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/Parameters/Parameters.cpp
    Source/Parameters/Parameters.h)

target_compile_definitions(RemixSafeMaster
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(RemixSafeMaster
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(BAKUON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
