cmake_minimum_required(VERSION 3.22)

project(SimpleVocals VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Use local JUCE if available, otherwise fetch ---
set(JUCE_LOCAL_PATH "$ENV{HOME}/JUCE" CACHE PATH "Path to local JUCE installation")
if(EXISTS "${JUCE_LOCAL_PATH}/CMakeLists.txt")
    message(STATUS "Using local JUCE at ${JUCE_LOCAL_PATH}")
    add_subdirectory(${JUCE_LOCAL_PATH} ${CMAKE_BINARY_DIR}/JUCE)
else()
    message(STATUS "Local JUCE not found at ${JUCE_LOCAL_PATH}, fetching...")
    include(FetchContent)
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.13
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(SimpleVocals
    COMPANY_NAME "echoe"
    PLUGIN_MANUFACTURER_CODE Echo
    PLUGIN_CODE Svcl
    FORMATS VST3 Standalone
    PRODUCT_NAME "SimpleVocals"
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(SimpleVocals)

target_sources(SimpleVocals
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/EffectChain.cpp
        Source/PresetManager.cpp
        Source/UI/PresetBar.cpp
        Source/UI/EQCurveComponent.cpp
        Source/UI/EQControlsStrip.cpp
        Source/UI/ChainStrip.cpp
        Source/UI/AutoChainButton.cpp
        Source/UI/DeEsserPanel.cpp
        Source/UI/DenoiseBar.cpp
        Source/UI/CompressorPanel.cpp
        Source/UI/SaturationPanel.cpp
        Source/UI/ReverbPanel.cpp
        Source/UI/HarmonizerPanel.cpp
        Source/UI/AutotuneComponent.cpp
        Source/UI/DelayPanel.cpp
        Source/Modules/DelayModule.cpp
        Source/Modules/NoiseReductionModule.cpp
        Source/Modules/EQModule.cpp
        Source/Modules/DeEsserModule.cpp
        Source/Modules/CompressorModule.cpp
        Source/Modules/SaturationModule.cpp
        Source/Modules/HarmonizerModule.cpp
        Source/Modules/ReverbModule.cpp
        Source/Modules/AutotuneModule.cpp
)

target_compile_definitions(SimpleVocals
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SimpleVocals
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
