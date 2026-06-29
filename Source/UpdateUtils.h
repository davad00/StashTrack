#pragma once

#include <juce_core/juce_core.h>

namespace StashTrack
{
    struct LatestReleaseInfo
    {
        bool valid = false;
        juce::String versionTag;
        juce::String releasePageUrl;
        juce::String installerUrl;
    };

    struct UpdateCheckResult
    {
        bool succeeded = false;
        bool updateAvailable = false;
        LatestReleaseInfo latest;
        juce::String message;
    };

    struct UpdateInstallResult
    {
        bool succeeded = false;
        juce::File installerFile;
        juce::String message;
    };

    juce::String normaliseVersionTag (const juce::String& version);
    bool isVersionNewer (const juce::String& currentVersion,
                         const juce::String& candidateVersion);
    juce::String getReleaseChangelogUrl (const LatestReleaseInfo& release);
    LatestReleaseInfo parseLatestReleaseJson (const juce::String& jsonText);
    LatestReleaseInfo fetchLatestReleaseInfo();
    UpdateCheckResult checkForUpdate (const juce::String& currentVersion);
    juce::File getUpdaterDownloadFile (const juce::String& versionTag,
                                       const juce::File& downloadFolder = {});
    UpdateInstallResult downloadInstallerToFile (const juce::String& installerUrl,
                                                 const juce::File& destinationFile);
    bool launchInstaller (const juce::File& installerFile);
}
