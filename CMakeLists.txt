cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CarTest VERSION 1.0.0)

add_subdirectory(JUCE)

juce_add_plugin(CarTest
    COMPANY_NAME "CarTestAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Crtx
    PLUGIN_CODE Crts
    FORMATS AU VST3 Standalone                  # AAX requires the Avid SDK — add later
    PRODUCT_NAME "Car Test"
    DESCRIPTION "Save yourself the trip to the driveway."
)

juce_generate_juce_header(CarTest)

juce_add_binary_data(CarTestData
    SOURCES
        Resources/bmw.jpg
        Resources/sedan_ir.wav
        Resources/phone_ir.wav
        Resources/laptop_ir.wav
        Resources/bt_speaker_ir.wav
)

target_sources(CarTest
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/EnvironmentProcessor.cpp
        Source/DSP/NoiseGenerator.cpp
)

target_compile_definitions(CarTest
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_features(CarTest PRIVATE cxx_std_20)

target_link_libraries(CarTest
    PRIVATE
        CarTestData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
