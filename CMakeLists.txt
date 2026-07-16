cmake_minimum_required(VERSION 3.22)

project(StashTrack VERSION 0.6.0)

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

enable_testing()

set(YOUTUBE_GRABBER_FORMATS VST3 Standalone)
option(STASHTRACK_COPY_PLUGIN_AFTER_BUILD
       "Copy the built plug-in to the system plug-in folder after building"
       OFF)

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
        Source/UpdateUtils.cpp
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

set(VSREACT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vsreact"
    CACHE PATH
    "Path to the VSReacT framework")

if (NOT EXISTS "${VSREACT_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "VSReacT not found at ${VSREACT_DIR}. Set VSREACT_DIR to the framework checkout.")
endif()

add_subdirectory("${VSREACT_DIR}" vsreact-build)
target_link_libraries(StashTrack PRIVATE vsreact)
target_compile_definitions(StashTrack PRIVATE
    STASHTRACK_HAS_VSREACT=1
    STASHTRACK_VSREACT_BUNDLE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/jsui-vsreact/build/main.js")

option(STASHTRACK_VSREACT_DEV
       "Load the UI bundle from the source tree and hot-reload it on change (turn OFF for installer builds; embeds the bundle instead)"
       ON)

if (STASHTRACK_VSREACT_DEV)
    target_compile_definitions(StashTrack PRIVATE STASHTRACK_VSREACT_DEV=1)
else()
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/jsui-vsreact/build/main.js")
        message(FATAL_ERROR "jsui-vsreact/build/main.js is missing. Build the UI bundle (cd jsui-vsreact; bun install; bun run build) before a production configure.")
    endif()
    juce_add_binary_data(StashTrackUiData
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/jsui-vsreact/build/main.js")
    target_link_libraries(StashTrack PRIVATE StashTrackUiData)
endif()

find_program(BUN_EXECUTABLE NAMES bun)

if (BUN_EXECUTABLE)
    add_custom_target(StashTrackUiBundle
        COMMAND "${BUN_EXECUTABLE}" install
        COMMAND "${BUN_EXECUTABLE}" run build
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/jsui-vsreact"
        COMMENT "Building the StashTrack VSReacT UI bundle with Bun")
else()
    message(STATUS "Bun was not found; run jsui-vsreact builds manually after installing Bun.")
endif()

juce_add_console_app(StashTrackTests
    PRODUCT_NAME "StashTrack Tests")

target_sources(StashTrackTests
    PRIVATE
        Source/DownloadUtils.cpp
        Source/UpdateUtils.cpp
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
