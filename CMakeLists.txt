cmake_minimum_required(VERSION 3.22)

project(StashTrack VERSION 0.1.0)

# Pull in JUCE. This workspace has JUCE downloaded next to this project at
# H:/code/VSTs/JUCE, but the cache variable lets other machines point at their
# own JUCE checkout.
set(JUCE_SOURCE_DIR "H:/code/VSTs/JUCE" CACHE PATH "Path to a JUCE checkout")

if (EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_SOURCE_DIR}" JUCE)
elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/CMakeLists.txt")
    add_subdirectory(libs/JUCE)
else()
    message(FATAL_ERROR "Set JUCE_SOURCE_DIR to a valid JUCE checkout.")
endif()

set(YOUTUBE_GRABBER_FORMATS VST3 Standalone)
option(STASHTRACK_COPY_PLUGIN_AFTER_BUILD
       "Copy the built plug-in to the system plug-in folder after building"
       OFF)
option(STASHTRACK_ENABLE_REACT_JUCE
       "Compile the vendored React-JUCE module into StashTrack"
       ON)
set(REACT_JUCE_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/external/react-juce"
    CACHE PATH
    "Path to a React-JUCE checkout")

if (APPLE)
    list(APPEND YOUTUBE_GRABBER_FORMATS AU)
endif()

juce_add_plugin(StashTrack
    COMPANY_NAME              "N9 Records"
    COMPANY_WEBSITE           "https://stashtrack.n9records.com"
    COMPANY_EMAIL             "vsts@n9records.com"
    COMPANY_COPYRIGHT         "Copyright (c) 2026 N9 Records"
    BUNDLE_ID                 "com.n9records.stashtrack"
    DESCRIPTION               "Download, clip, preview, and drag audio from yt-dlp-supported URLs into your DAW."
    PLUGIN_MANUFACTURER_CODE  Ycmp
    PLUGIN_CODE               StTk
    FORMATS                   ${YOUTUBE_GRABBER_FORMATS}
    PRODUCT_NAME              "StashTrack"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    COPY_PLUGIN_AFTER_BUILD   ${STASHTRACK_COPY_PLUGIN_AFTER_BUILD})

target_sources(StashTrack
    PRIVATE
        Source/DownloadUtils.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_compile_definitions(StashTrack
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(StashTrack
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (STASHTRACK_ENABLE_REACT_JUCE)
    if (EXISTS "${REACT_JUCE_SOURCE_DIR}/react_juce/react_juce.h")
        juce_add_module("${REACT_JUCE_SOURCE_DIR}/react_juce")
        target_include_directories(StashTrack PRIVATE
            "${REACT_JUCE_SOURCE_DIR}/react_juce")
        target_compile_definitions(StashTrack PRIVATE
            STASHTRACK_HAS_REACT_JUCE=1
            REACTJUCE_USE_DUKTAPE=1
            STASHTRACK_REACT_JUCE_BUNDLE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/jsui/build/js/main.js")
        target_link_libraries(StashTrack PRIVATE react_juce)

        if (MSVC)
            target_compile_options(StashTrack PRIVATE
                /wd4100
                /wd4127
                /wd4244
                /wd4505
                /wd4611
                /wd4702
                /wd4996)
        endif()
    else()
        message(WARNING "React-JUCE not found at ${REACT_JUCE_SOURCE_DIR}; building native JUCE UI only.")
    endif()
endif()

find_program(BUN_EXECUTABLE NAMES bun)

if (BUN_EXECUTABLE)
    add_custom_target(StashTrackReactJuceUiBundle
        COMMAND "${BUN_EXECUTABLE}" install
        COMMAND "${BUN_EXECUTABLE}" run build
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/jsui"
        COMMENT "Building StashTrack React-JUCE UI bundle with Bun")
else()
    message(STATUS "Bun was not found; run jsui builds manually after installing Bun.")
endif()

juce_add_console_app(StashTrackTests
    PRODUCT_NAME "StashTrack Tests")

target_sources(StashTrackTests
    PRIVATE
        Source/DownloadUtils.cpp
        Tests/DownloadUtilsTests.cpp)

target_compile_definitions(StashTrackTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(StashTrackTests
    PRIVATE
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if (WIN32)
    target_link_libraries(StashTrack PRIVATE Shell32 User32)
    target_link_libraries(StashTrackTests PRIVATE Shell32 User32)
endif()

enable_testing()
add_test(NAME StashTrackDownloadUtils COMMAND StashTrackTests)

if (WIN32)
    find_program(POWERSHELL_EXECUTABLE NAMES powershell pwsh)

    if (POWERSHELL_EXECUTABLE)
        add_test(NAME StashTrackPackaging
                 COMMAND ${POWERSHELL_EXECUTABLE}
                         -NoProfile
                         -ExecutionPolicy Bypass
                         -File "${CMAKE_CURRENT_SOURCE_DIR}/Tests/PackagingTests.ps1")
    endif()
endif()
