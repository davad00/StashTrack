#pragma once

#include <juce_core/juce_core.h>

namespace StashTrack
{
    // Legal notice: downloading media from YouTube or other sites may violate
    // terms of service or copyright laws. Only download content you have the
    // rights to use.
    struct YtDlpOutputAnalysis
    {
        bool hasError = false;
        juce::String message;
        juce::File downloadedFile;
    };

    struct DownloadJobResult
    {
        bool succeeded = false;
        juce::File downloadedFile;
        juce::String message;
        juce::String processOutput;
        juce::uint32 exitCode = 0;
    };

    struct DownloadFolderChoice
    {
        juce::File folder;
        juce::File flStudioProjectFile;
        juce::String sourceDescription;
        bool fromFlStudioProject = false;
    };

    struct DownloadSection
    {
        bool enabled = false;
        juce::String startTime;
        juce::String endTime;
    };

    struct DownloadOptions
    {
        DownloadSection section;
    };

    struct TimecodeParseResult
    {
        bool valid = false;
        double seconds = 0.0;
    };

    struct DownloadSectionValidation
    {
        bool valid = true;
        juce::String message;
        DownloadSection section;
    };

    bool isLikelySupportedUrl (const juce::String& text);
    juce::File getFallbackDownloadsDirectory();
    bool isLikelyUnsavedFlStudioProjectName (const juce::String& projectName);
    juce::File chooseFlStudioProjectFileFromMruPaths (const juce::StringArray& mruPaths);
    juce::File chooseFlStudioProjectFileFromMruPaths (const juce::StringArray& mruPaths,
                                                      const juce::StringArray& activeWindowTitles);
    juce::File findRecentFlStudioProjectFile();
    DownloadFolderChoice chooseDownloadFolder (const juce::File& flStudioProjectFile,
                                               const juce::File& fallbackDownloadsDirectory);
    DownloadFolderChoice getDownloadFolderChoice();
    juce::File getDownloadDirectory();
    TimecodeParseResult parseTimecodeToSeconds (juce::String text);
    DownloadSectionValidation validateDownloadSection (bool enabled,
                                                       const juce::String& startTime,
                                                       const juce::String& endTime);
    juce::StringArray buildYtDlpCommand (const juce::String& url,
                                         const juce::File& downloadFolder,
                                         const DownloadOptions& options,
                                         const juce::File& toolDirectory = {});
    juce::StringArray buildYtDlpCommand (const juce::String& url,
                                         const juce::File& downloadFolder,
                                         const juce::File& toolDirectory = {});
    YtDlpOutputAnalysis analyseYtDlpOutput (const juce::String& output,
                                            const juce::File& downloadFolder);
    juce::File findNewestSupportedAudioFile (const juce::File& downloadFolder);
    DownloadJobResult downloadAudioWithYtDlp (const juce::String& url,
                                              const juce::File& downloadFolder,
                                              const DownloadOptions& options);
    DownloadJobResult downloadAudioWithYtDlp (const juce::String& url,
                                              const juce::File& downloadFolder);
}
