#include <juce_core/juce_core.h>

#include "../Source/DownloadUtils.h"
#include "../Source/UpdateUtils.h"

#include <iostream>

namespace
{
    int failures = 0;

    void expect (bool condition, const juce::String& message)
    {
        if (! condition)
        {
            ++failures;
            std::cerr << "FAIL: " << message << std::endl;
        }
    }

    void commandContainsExpectedYtDlpOptions()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("StashTrackTests");
        const auto url = juce::String ("https://www.youtube.com/watch?v=dQw4w9WgXcQ");
        const auto command = StashTrack::buildYtDlpCommand (url, folder);

        expect (command.size() > 10, "command should contain yt-dlp arguments");
        expect (command[0].containsIgnoreCase ("uvx"), "first argument should launch uvx");
        expect (command.contains ("--refresh-package"),
                "uvx command should refresh yt-dlp before launch");
        expect (command[command.indexOf ("--refresh-package") + 1] == "yt-dlp",
                "uvx refresh should target the yt-dlp package");
        expect (command.contains ("--from"), "uvx command should choose the PyPI package explicitly");
        expect (command[command.indexOf ("--from") + 1] == "yt-dlp@latest",
                "uvx command should install/run the latest yt-dlp package");
        expect (command[command.indexOf ("--from") + 2] == "yt-dlp",
                "uvx command should run the yt-dlp console command");
        expect (command.contains ("-f"), "command should request an explicit format");
        expect (command[command.indexOf ("-f") + 1] == "bestaudio", "command should request bestaudio");
        expect (command.contains ("-x"), "command should extract audio");
        expect (command[command.indexOf ("--audio-format") + 1] == "wav",
                "command should convert to wav for JUCE AudioFormatManager compatibility");
        expect (command[command.indexOf ("--print") + 1] == "after_move:filepath",
                "command should print the final moved file path");
        expect (command[command.indexOf ("--output") + 1].endsWith ("%(title)s.%(ext)s"),
                "output template should preserve the yt-dlp title and extension");
        expect (command[command.size() - 1] == url, "last command argument should be the URL");
    }

    void commandContainsDownloadSectionWhenRequested()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("StashTrackTests");
        StashTrack::DownloadOptions options;
        options.section.enabled = true;
        options.section.startTime = "1:02";
        options.section.endTime = "1:32.5";

        const auto command = StashTrack::buildYtDlpCommand (
            "https://www.youtube.com/watch?v=dQw4w9WgXcQ",
            folder,
            options);

        expect (command.contains ("--download-sections"),
                "section downloads should pass yt-dlp's section option");
        expect (command[command.indexOf ("--download-sections") + 1] == "*1:02-1:32.5",
                "section download should use yt-dlp's *start-end syntax");
        expect (command[command.indexOf ("-f") + 1] == "best[protocol*=m3u8][height<=360]/best[protocol*=m3u8]/bestaudio",
                "section downloads should prefer seekable m3u8 segment streams before falling back to bestaudio");
        expect (! command.contains ("--downloader"),
                "timed sections automatically use yt-dlp's ffmpeg downloader when needed");
        expect (! command.contains ("--force-keyframes-at-cuts"),
                "audio section downloads should not force re-encoding keyframes");
    }

    void longYoutubeClipUsesFfmpegSectionDownload()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("StashTrackTests");
        StashTrack::DownloadOptions options;
        options.section.enabled = true;
        options.section.startTime = "1:26:23";
        options.section.endTime = "1:27:00";

        const auto command = StashTrack::buildYtDlpCommand (
            "https://www.youtube.com/watch?v=hsGOT_0L16U",
            folder,
            options);

        expect (command[command.indexOf ("--download-sections") + 1] == "*1:26:23-1:27:00",
                "long YouTube clips should preserve hh:mm:ss section bounds");
        expect (command[command.indexOf ("-f") + 1].contains ("protocol*=m3u8"),
                "long YouTube clips should prefer segment streams so yt-dlp can avoid linear DASH seeks where available");
    }

    void clippedDownloadsIncludeTimeRangeInOutputFilename()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("StashTrackTests");

        StashTrack::DownloadOptions firstClip;
        firstClip.section.enabled = true;
        firstClip.section.startTime = "1:23:45";
        firstClip.section.endTime = "1:24:00";

        StashTrack::DownloadOptions secondClip;
        secondClip.section.enabled = true;
        secondClip.section.startTime = "1:24:01";
        secondClip.section.endTime = "1:24:30";

        const auto firstCommand = StashTrack::buildYtDlpCommand (
            "https://www.youtube.com/watch?v=hsGOT_0L16U",
            folder,
            firstClip);

        const auto secondCommand = StashTrack::buildYtDlpCommand (
            "https://www.youtube.com/watch?v=hsGOT_0L16U",
            folder,
            secondClip);

        const auto firstOutput = firstCommand[firstCommand.indexOf ("--output") + 1];
        const auto secondOutput = secondCommand[secondCommand.indexOf ("--output") + 1];
        const auto firstFileName = juce::File (firstOutput).getFileName();
        const auto secondFileName = juce::File (secondOutput).getFileName();

        expect (firstFileName.contains ("[clip 1-23-45 to 1-24-00]"),
                "clipped output filename should include a Windows-safe start/end range");
        expect (secondFileName.contains ("[clip 1-24-01 to 1-24-30]"),
                "a second clip from the same source should get its own output filename");
        expect (firstOutput != secondOutput,
                "different clip ranges from the same source should not reuse the same yt-dlp output path");
        expect (! firstFileName.containsChar (':') && ! secondFileName.containsChar (':'),
                "clip time ranges in filenames must not contain Windows-invalid colon characters");
    }

    void commandPrefersBundledYtDlpForYoutubeEjsWhenAvailable()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("StashTrackTests");
        const auto toolDirectory = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                       .getChildFile ("StashTrackToolTests");
        toolDirectory.createDirectory();
        toolDirectory.getChildFile ("uvx.exe").replaceWithText ("fake uvx");
        toolDirectory.getChildFile ("uvx").replaceWithText ("fake uvx");
        toolDirectory.getChildFile ("yt-dlp.exe").replaceWithText ("fake yt-dlp");
        toolDirectory.getChildFile ("yt-dlp").replaceWithText ("fake yt-dlp");
        toolDirectory.getChildFile ("deno.exe").replaceWithText ("fake deno");
        toolDirectory.getChildFile ("deno").replaceWithText ("fake deno");

        StashTrack::DownloadOptions options;
        options.section.enabled = true;
        options.section.startTime = "1:23:45";
        options.section.endTime = "1:24:00";

        const auto command = StashTrack::buildYtDlpCommand (
            "https://www.youtube.com/watch?v=hsGOT_0L16U",
            folder,
            options,
            toolDirectory);

        expect (command[0].containsIgnoreCase ("yt-dlp"), "bundled yt-dlp.exe should be launched directly");
        expect (! command.contains ("--from"),
                "bundled yt-dlp command should not go through uvx package selection");
        expect (! command.contains ("--refresh-package"),
                "bundled yt-dlp command should not depend on uvx refresh behavior");
        expect (command.contains ("--js-runtimes"),
                "bundled Deno should be passed to bundled yt-dlp for YouTube JS challenge solving");
        expect (command[command.indexOf ("--js-runtimes") + 1].startsWithIgnoreCase ("deno:"),
                "Deno runtime argument should use yt-dlp's deno:path syntax");
        expect (command.contains ("--remote-components"),
                "Deno-based YouTube solving should enable yt-dlp's remote EJS component");
        expect (command[command.indexOf ("--remote-components") + 1] == "ejs:npm",
                "yt-dlp should fetch the EJS solver package through Deno/npm when needed");
    }

    void uvxFallbackDoesNotUseBundledDenoOnlyOptions()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("StashTrackTests");
        const auto toolDirectory = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                       .getChildFile ("StashTrackToolFallbackTests");
        toolDirectory.deleteRecursively();
        toolDirectory.createDirectory();
        toolDirectory.getChildFile ("uvx.exe").replaceWithText ("fake uvx");
        toolDirectory.getChildFile ("uvx").replaceWithText ("fake uvx");
        toolDirectory.getChildFile ("deno.exe").replaceWithText ("fake deno");
        toolDirectory.getChildFile ("deno").replaceWithText ("fake deno");

        StashTrack::DownloadOptions options;
        options.section.enabled = true;
        options.section.startTime = "25:30";
        options.section.endTime = "26:00";

        const auto command = StashTrack::buildYtDlpCommand (
            "https://www.youtube.com/watch?v=WxNNWhOHcsQ",
            folder,
            options,
            toolDirectory);

        expect (command[0].containsIgnoreCase ("uvx"), "uvx should remain the fallback when bundled yt-dlp is missing");
        expect (command[command.indexOf ("--from") + 1] == "yt-dlp@latest",
                "uvx fallback should still request the latest yt-dlp package");
        expect (! command.contains ("--js-runtimes"),
                "uvx fallback should not pass options that older resolved yt-dlp executables may reject");
        expect (! command.contains ("--remote-components"),
                "uvx fallback should not pass yt-dlp EJS options without a bundled yt-dlp executable");
    }

    void rejectsEmptyAndNonHttpUrls()
    {
        expect (! StashTrack::isLikelySupportedUrl (""), "empty URL should be rejected");
        expect (! StashTrack::isLikelySupportedUrl ("   "), "blank URL should be rejected");
        expect (! StashTrack::isLikelySupportedUrl ("not a url"), "plain text should be rejected");
        expect (! StashTrack::isLikelySupportedUrl ("ftp://example.com/audio"),
                "non-http URLs should be rejected");
        expect (StashTrack::isLikelySupportedUrl ("https://youtu.be/example"),
                "https URLs should be accepted");
        expect (StashTrack::isLikelySupportedUrl ("http://example.com/video"),
                "http URLs should be accepted");
    }

    void parsesErrorLinesFromYtDlpOutput()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory);
        const auto analysis = StashTrack::analyseYtDlpOutput (
            "[youtube] Extracting URL\nERROR: ffmpeg not found. Please install ffmpeg.\n",
            folder);

        expect (analysis.hasError, "ERROR lines should mark yt-dlp output as failed");
        expect (analysis.message.containsIgnoreCase ("ffmpeg not found"),
                "error summary should include the yt-dlp error text");
    }

    void parsesYtDlpCliErrorLines()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory);
        const auto analysis = StashTrack::analyseYtDlpOutput (
            "yt-dlp: error: no such option: --js-runtimes\n",
            folder);

        expect (analysis.hasError, "yt-dlp CLI error lines should mark output as failed");
        expect (analysis.message.containsIgnoreCase ("no such option: --js-runtimes"),
                "CLI error summary should include the unsupported option text");
    }

    void extractsPrintedDownloadedFileFromOutput()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("StashTrackTests");
        const auto expected = folder.getChildFile ("Downloaded Song.wav");
        const auto analysis = StashTrack::analyseYtDlpOutput (
            "[download] 100% of 10.00MiB\n" + expected.getFullPathName() + "\n",
            folder);

        expect (! analysis.hasError, "successful output should not be marked as failed");
        expect (analysis.downloadedFile == expected, "plain printed file path should be captured");
    }

    void defaultDownloadDirectoryIgnoresHostWorkingDirectory()
    {
        const auto oldWorkingDirectory = juce::File::getCurrentWorkingDirectory();
        const auto fakeHostWorkingDirectory = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                                  .getChildFile ("StashTrackFakeHostWorkingDirectory");
        fakeHostWorkingDirectory.createDirectory();
        fakeHostWorkingDirectory.setAsCurrentWorkingDirectory();

        const auto selectedDirectory = StashTrack::getDownloadDirectory();

        oldWorkingDirectory.setAsCurrentWorkingDirectory();

        expect (selectedDirectory != fakeHostWorkingDirectory,
                "default download directory should not use the DAW process working directory");
    }

    void flStudioMruChoosesFirstExistingFlp()
    {
        const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("StashTrackMruTests");
        const auto projectA = root.getChildFile ("First Project")
                              .getChildFile ("First Project.flp");
        const auto projectB = root.getChildFile ("Second Project")
                              .getChildFile ("Second Project.flp");

        projectA.getParentDirectory().createDirectory();
        projectB.getParentDirectory().createDirectory();
        projectA.replaceWithText ("fake flp");
        projectB.replaceWithText ("fake flp");

        juce::StringArray mruPaths;
        mruPaths.add (projectA.getFullPathName());
        mruPaths.add (projectB.getFullPathName());

        juce::StringArray windowTitles;
        windowTitles.add ("First Project - FL Studio 21");

        expect (StashTrack::chooseFlStudioProjectFileFromMruPaths (mruPaths, windowTitles) == projectA,
                "FL Studio MRU slot 0 should be used when the active FL window matches it");
    }

    void flStudioMruIgnoresMissingAndNonFlpEntries()
    {
        const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("StashTrackMruTests");
        const auto zipPackage = root.getChildFile ("Packed Project.zip");
        const auto project = root.getChildFile ("Existing Project")
                             .getChildFile ("Existing Project.flp");

        project.getParentDirectory().createDirectory();
        zipPackage.replaceWithText ("fake zip");
        project.replaceWithText ("fake flp");

        juce::StringArray mruPaths;
        mruPaths.add (root.getChildFile ("Missing.flp").getFullPathName());
        mruPaths.add (zipPackage.getFullPathName());
        mruPaths.add (project.getFullPathName());

        juce::StringArray windowTitles;
        windowTitles.add ("Existing Project - FL Studio 21");

        expect (StashTrack::chooseFlStudioProjectFileFromMruPaths (mruPaths, windowTitles) == project,
                "FL Studio MRU resolver should skip missing files and zipped packages");
    }

    void flStudioMruFallsBackWhenProjectLooksUnsaved()
    {
        const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("StashTrackMruTests");
        const auto project = root.getChildFile ("Old Saved Project")
                             .getChildFile ("Old Saved Project.flp");
        const auto untitledProject = root.getChildFile ("Untitled Folder")
                                     .getChildFile ("untitled.flp");

        project.getParentDirectory().createDirectory();
        untitledProject.getParentDirectory().createDirectory();
        project.replaceWithText ("fake flp");
        untitledProject.replaceWithText ("fake flp");

        juce::StringArray mruPaths;
        mruPaths.add (project.getFullPathName());
        mruPaths.add (untitledProject.getFullPathName());

        juce::StringArray unsavedWindowTitles;
        unsavedWindowTitles.add ("Untitled - FL Studio 21");

        expect (StashTrack::chooseFlStudioProjectFileFromMruPaths (mruPaths, unsavedWindowTitles) == juce::File(),
                "unsaved FL projects should not reuse the last saved MRU project folder");

        juce::StringArray untitledWindowTitles;
        untitledWindowTitles.add ("untitled - FL Studio 21");

        expect (StashTrack::chooseFlStudioProjectFileFromMruPaths (juce::StringArray (untitledProject.getFullPathName()),
                                                                   untitledWindowTitles) == juce::File(),
                "an MRU file named untitled.flp should not be trusted as a saved project");
    }

    void validatesClipTimeRanges()
    {
        const auto seconds = StashTrack::parseTimecodeToSeconds ("1:02:03.5");
        expect (seconds.valid, "hh:mm:ss.s timecodes should parse");
        expect (std::abs (seconds.seconds - 3723.5) < 0.001,
                "parsed hh:mm:ss.s seconds should be correct");

        expect (! StashTrack::validateDownloadSection (true, "", "0:30").valid,
                "clip mode should require a start time");
        expect (! StashTrack::validateDownloadSection (true, "1:00", "0:30").valid,
                "clip end must be after clip start");
        expect (StashTrack::validateDownloadSection (false, "", "").valid,
                "disabled clip mode should not require times");

        const auto valid = StashTrack::validateDownloadSection (true, "0:10", "0:40");
        expect (valid.valid, "valid clip start/end should be accepted");
        expect (valid.section.enabled, "valid clip result should enable the section");
    }

    void downloadFolderChoicePrefersProjectParent()
    {
        const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("StashTrackFolderChoiceTests");
        const auto fallback = root.getChildFile ("Downloads");
        const auto project = root.getChildFile ("Project Folder")
                             .getChildFile ("Song.flp");

        project.getParentDirectory().createDirectory();
        project.replaceWithText ("fake flp");

        const auto choice = StashTrack::chooseDownloadFolder (project, fallback);

        expect (choice.folder == project.getParentDirectory(),
                "download folder should be the saved FLP parent folder when a saved FLP is known");
        expect (choice.fromFlStudioProject,
                "download folder choice should report that it came from the FL Studio project");
    }

    void downloadFolderChoiceFallsBackWithoutProject()
    {
        const auto fallback = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                  .getChildFile ("StashTrackFallbackDownloads");
        const auto choice = StashTrack::chooseDownloadFolder ({}, fallback);

        expect (choice.folder == fallback,
                "download folder should fall back to the browser downloads folder without a saved FLP");
        expect (! choice.fromFlStudioProject,
                "fallback download folder choice should not report an FL Studio project");
    }

    void normalisesVersionTagsForDisplayAndComparison()
    {
        expect (StashTrack::normaliseVersionTag ("v0.4.0") == "0.4.0",
                "version normalisation should strip a leading v");
        expect (StashTrack::normaliseVersionTag ("  0.5  ") == "0.5",
                "version normalisation should trim whitespace");
        expect (StashTrack::normaliseVersionTag ("release-v1.2") == "1.2",
                "version normalisation should skip non-numeric prefixes");
    }

    void comparesSemanticVersions()
    {
        expect (StashTrack::isVersionNewer ("0.4.0", "v0.5"),
                "v0.5 should be newer than 0.4.0");
        expect (StashTrack::isVersionNewer ("0.4.9", "v0.5.0"),
                "minor version bumps should be newer");
        expect (! StashTrack::isVersionNewer ("0.4.0", "v0.4"),
                "matching versions with missing patch should not count as newer");
        expect (! StashTrack::isVersionNewer ("0.5.0", "v0.4.9"),
                "older releases should not count as newer");
    }

    void parsesGitHubLatestReleaseJson()
    {
        const auto release = StashTrack::parseLatestReleaseJson (R"json(
            {
              "tag_name": "v0.5",
              "html_url": "https://github.com/davad00/StashTrack/releases/tag/v0.5",
              "assets": [
                {
                  "name": "Source.zip",
                  "browser_download_url": "https://example.com/source.zip"
                },
                {
                  "name": "StashTrackv0.5Setup.exe",
                  "browser_download_url": "https://github.com/davad00/StashTrack/releases/download/v0.5/StashTrackv0.5Setup.exe"
                }
              ]
            }
        )json");

        expect (release.valid, "GitHub release JSON with an installer asset should be valid");
        expect (release.versionTag == "v0.5", "release parser should keep the GitHub tag");
        expect (release.installerUrl.endsWith ("StashTrackv0.5Setup.exe"),
                "release parser should select the Windows setup asset");
        expect (release.releasePageUrl.endsWith ("/v0.5"),
                "release parser should keep the release page URL");
    }

    void releaseInfoExposesHttpsChangelogUrl()
    {
        StashTrack::LatestReleaseInfo release;
        release.releasePageUrl = " https://github.com/davad00/StashTrack/releases/tag/v0.6 ";

        expect (StashTrack::getReleaseChangelogUrl (release).endsWith ("/v0.6"),
                "updater prompt should expose the GitHub release page as the changelog URL");

        release.releasePageUrl = "http://example.com/not-secure";

        expect (StashTrack::getReleaseChangelogUrl (release).isEmpty(),
                "updater prompt should not expose a non-https changelog URL");
    }

    void rejectsReleaseJsonWithoutInstaller()
    {
        const auto release = StashTrack::parseLatestReleaseJson (R"json(
            {
              "tag_name": "v0.5",
              "assets": [
                {
                  "name": "notes.txt",
                  "browser_download_url": "https://example.com/notes.txt"
                }
              ]
            }
        )json");

        expect (! release.valid, "release parser should reject releases with no setup exe asset");
    }

    void updaterDownloadFileUsesDownloadsFolderAndVersion()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                .getChildFile ("StashTrackUpdaterTests");
        const auto file = StashTrack::getUpdaterDownloadFile ("v0.5", folder);

        expect (file.getParentDirectory() == folder,
                "updater installer should be staged in the selected folder");
        expect (file.getFileName() == "StashTrackv0.5Setup.exe",
                "updater installer filename should include the release tag");
    }
}

int main()
{
    commandContainsExpectedYtDlpOptions();
    commandContainsDownloadSectionWhenRequested();
    longYoutubeClipUsesFfmpegSectionDownload();
    clippedDownloadsIncludeTimeRangeInOutputFilename();
    commandPrefersBundledYtDlpForYoutubeEjsWhenAvailable();
    uvxFallbackDoesNotUseBundledDenoOnlyOptions();
    rejectsEmptyAndNonHttpUrls();
    parsesErrorLinesFromYtDlpOutput();
    parsesYtDlpCliErrorLines();
    extractsPrintedDownloadedFileFromOutput();
    defaultDownloadDirectoryIgnoresHostWorkingDirectory();
    flStudioMruChoosesFirstExistingFlp();
    flStudioMruIgnoresMissingAndNonFlpEntries();
    flStudioMruFallsBackWhenProjectLooksUnsaved();
    validatesClipTimeRanges();
    downloadFolderChoicePrefersProjectParent();
    downloadFolderChoiceFallsBackWithoutProject();
    normalisesVersionTagsForDisplayAndComparison();
    comparesSemanticVersions();
    parsesGitHubLatestReleaseJson();
    releaseInfoExposesHttpsChangelogUrl();
    rejectsReleaseJsonWithoutInstaller();
    updaterDownloadFileUsesDownloadsFolderAndVersion();

    if (failures == 0)
    {
        std::cout << "All DownloadUtils tests passed." << std::endl;
        return 0;
    }

    std::cerr << failures << " DownloadUtils test failure(s)." << std::endl;
    return 1;
}
