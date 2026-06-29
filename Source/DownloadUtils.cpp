#include "DownloadUtils.h"

#if JUCE_WINDOWS
 #include <ShlObj.h>
 #include <windows.h>
#endif

#include <cmath>
#include <cstdlib>

namespace StashTrack
{
namespace
{
    constexpr auto supportedAudioExtensions = ".wav;.aif;.aiff;.flac;.mp3;.ogg;.m4a;.aac";

    juce::String executableFileName (const juce::String& baseName)
    {
       #if JUCE_WINDOWS
        if (! baseName.endsWithIgnoreCase (".exe"))
            return baseName + ".exe";
       #endif

        return baseName;
    }

    juce::Array<juce::File> getToolSearchFolders (const juce::File& explicitToolDirectory)
    {
        juce::Array<juce::File> folders;

        if (explicitToolDirectory.exists())
            folders.add (explicitToolDirectory);

        const auto currentApp = juce::File::getSpecialLocation (juce::File::currentApplicationFile);
        const auto currentExe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

        folders.add (currentApp.isDirectory() ? currentApp : currentApp.getParentDirectory());
        folders.add (currentExe.isDirectory() ? currentExe : currentExe.getParentDirectory());
        folders.add (juce::File::getCurrentWorkingDirectory());

        return folders;
    }

    juce::File findBundledExecutable (const juce::String& baseName,
                                      const juce::File& explicitToolDirectory = {})
    {
        const auto fileName = executableFileName (baseName);

        for (const auto& folder : getToolSearchFolders (explicitToolDirectory))
        {
            const auto candidate = folder.getChildFile (fileName);

            if (candidate.existsAsFile())
                return candidate;
        }

        return {};
    }

    juce::String resolveExecutableForChildProcess (const juce::String& baseName,
                                                   const juce::File& explicitToolDirectory = {})
    {
        if (const auto bundled = findBundledExecutable (baseName, explicitToolDirectory);
            bundled.existsAsFile())
        {
            return bundled.getFullPathName();
        }

        return baseName;
    }

    juce::String summariseOutput (const juce::String& output)
    {
        auto lines = juce::StringArray::fromLines (output);

        for (auto line : lines)
        {
            line = line.trim();

            if (line.containsIgnoreCase ("ERROR:"))
                return line;
        }

        for (auto line : lines)
        {
            line = line.trim();

            if (line.isNotEmpty())
                return line.substring (0, 240);
        }

        return "yt-dlp did not return any output.";
    }

    bool isSupportedAudioFilePath (const juce::String& possiblePath)
    {
        return juce::File (possiblePath).hasFileExtension (supportedAudioExtensions);
    }

    juce::String extractPathFromYtDlpLine (juce::String line)
    {
        line = line.trim().unquoted();

        if (line.contains ("Destination:"))
            line = line.fromFirstOccurrenceOf ("Destination:", false, false).trim().unquoted();

        if (line.startsWithIgnoreCase ("file:"))
            line = line.fromFirstOccurrenceOf (":", false, false).trim().unquoted();

        return line;
    }

    juce::File fileFromOutputLine (const juce::String& line, const juce::File& downloadFolder)
    {
        const auto possiblePath = extractPathFromYtDlpLine (line);

        if (! isSupportedAudioFilePath (possiblePath))
            return {};

        if (juce::File::isAbsolutePath (possiblePath))
            return juce::File (possiblePath);

        return downloadFolder.getChildFile (possiblePath);
    }

    bool parseNonNegativeNumber (const juce::String& text, double& value)
    {
        const auto trimmed = text.trim();

        if (trimmed.isEmpty())
            return false;

        const auto raw = trimmed.toStdString();
        char* end = nullptr;
        value = std::strtod (raw.c_str(), &end);

        return end != raw.c_str()
            && end != nullptr
            && *end == '\0'
            && std::isfinite (value)
            && value >= 0.0;
    }

    juce::String makeClipTimeFilenameSafe (juce::String time)
    {
        return time.trim()
                   .replaceCharacters (":/\\*?\"<>|", "---------")
                   .replaceCharacter (' ', '-');
    }

    juce::String getOutputTemplateName (const DownloadOptions& options)
    {
        if (! options.section.enabled)
            return "%(title)s.%(ext)s";

        const auto start = makeClipTimeFilenameSafe (options.section.startTime);
        const auto end = makeClipTimeFilenameSafe (options.section.endTime);

        return "%(title)s [clip " + start + " to " + end + "].%(ext)s";
    }

    bool activeTitleLooksUnsaved (const juce::String& title)
    {
        return title.containsIgnoreCase ("FL Studio")
            && (title.containsIgnoreCase ("untitled")
                || title.containsIgnoreCase ("new project"));
    }

    bool titleMatchesProjectFile (const juce::String& title, const juce::File& projectFile)
    {
        if (! title.containsIgnoreCase ("FL Studio"))
            return false;

        const auto fileName = projectFile.getFileName();
        const auto stem = projectFile.getFileNameWithoutExtension();

        return stem.isNotEmpty()
            && ! isLikelyUnsavedFlStudioProjectName (stem)
            && (title.containsIgnoreCase (fileName)
                || title.containsIgnoreCase (stem));
    }

   #if JUCE_WINDOWS
    BOOL CALLBACK collectVisibleWindowTitle (HWND windowHandle, LPARAM userData)
    {
        if (! IsWindowVisible (windowHandle))
            return TRUE;

        const auto length = GetWindowTextLengthW (windowHandle);

        if (length <= 0)
            return TRUE;

        juce::HeapBlock<wchar_t> buffer (static_cast<size_t> (length) + 1);
        GetWindowTextW (windowHandle, buffer, length + 1);

        const juce::String title (buffer.getData());

        if (title.containsIgnoreCase ("FL Studio")
            || title.containsIgnoreCase ("StashTrack"))
        {
            auto* titles = reinterpret_cast<juce::StringArray*> (userData);
            titles->add (title);
        }

        return TRUE;
    }

    juce::StringArray getVisibleFlStudioWindowTitles()
    {
        juce::StringArray titles;
        EnumWindows (collectVisibleWindowTitle, reinterpret_cast<LPARAM> (&titles));
        return titles;
    }

    juce::File getWindowsKnownDownloadsDirectory()
    {
        PWSTR rawPath = nullptr;
        const auto result = SHGetKnownFolderPath (FOLDERID_Downloads,
                                                  KF_FLAG_DEFAULT,
                                                  nullptr,
                                                  &rawPath);

        if (FAILED (result) || rawPath == nullptr)
            return {};

        const juce::File downloads { juce::String (rawPath) };
        CoTaskMemFree (rawPath);

        if (downloads.exists() || downloads.createDirectory().wasOk())
            return downloads;

        return {};
    }

    juce::StringArray getFlStudioMruPathsFromRegistry()
    {
        juce::StringArray paths;

        for (int version = 30; version >= 1; --version)
        {
            const auto key = "HKEY_CURRENT_USER\\Software\\Image-Line\\FL Studio "
                           + juce::String (version)
                           + "\\MRU\\";

            for (int index = 0; index < 64; ++index)
            {
                const auto value = juce::WindowsRegistry::getValue (key + juce::String (index), {});

                if (value.isNotEmpty())
                    paths.add (value);
            }

            if (! paths.isEmpty())
                return paths;
        }

        return paths;
    }
   #endif
}

bool isLikelySupportedUrl (const juce::String& text)
{
    const auto trimmed = text.trim();

    return trimmed.startsWithIgnoreCase ("https://")
        || trimmed.startsWithIgnoreCase ("http://");
}

juce::File getFallbackDownloadsDirectory()
{
   #if JUCE_WINDOWS
    if (const auto knownDownloads = getWindowsKnownDownloadsDirectory();
        knownDownloads != juce::File())
    {
        return knownDownloads;
    }
   #endif

    const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    auto downloads = home.getChildFile ("Downloads");

    if (downloads.exists() || downloads.createDirectory().wasOk())
        return downloads;

    auto documents = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    documents.createDirectory();
    return documents;
}

bool isLikelyUnsavedFlStudioProjectName (const juce::String& projectName)
{
    const auto name = projectName.trim().toLowerCase();

    return name.isEmpty()
        || name == "untitled"
        || name.startsWith ("untitled ")
        || name.startsWith ("untitled-")
        || name.startsWith ("new project");
}

juce::File chooseFlStudioProjectFileFromMruPaths (const juce::StringArray& mruPaths)
{
    for (const auto& path : mruPaths)
    {
        const juce::File candidate { path.trim().unquoted() };

        if (candidate.hasFileExtension (".flp") && candidate.existsAsFile())
            return candidate;
    }

    return {};
}

juce::File chooseFlStudioProjectFileFromMruPaths (const juce::StringArray& mruPaths,
                                                  const juce::StringArray& activeWindowTitles)
{
    if (activeWindowTitles.isEmpty())
        return {};

    for (const auto& title : activeWindowTitles)
        if (activeTitleLooksUnsaved (title))
            return {};

    for (const auto& path : mruPaths)
    {
        const juce::File candidate { path.trim().unquoted() };

        if (! candidate.hasFileExtension (".flp")
            || ! candidate.existsAsFile()
            || isLikelyUnsavedFlStudioProjectName (candidate.getFileNameWithoutExtension()))
        {
            continue;
        }

        for (const auto& title : activeWindowTitles)
            if (titleMatchesProjectFile (title, candidate))
                return candidate;
    }

    return {};
}

juce::File findRecentFlStudioProjectFile()
{
   #if JUCE_WINDOWS
    return chooseFlStudioProjectFileFromMruPaths (getFlStudioMruPathsFromRegistry(),
                                                 getVisibleFlStudioWindowTitles());
   #else
    return {};
   #endif
}

DownloadFolderChoice chooseDownloadFolder (const juce::File& flStudioProjectFile,
                                           const juce::File& fallbackDownloadsDirectory)
{
    DownloadFolderChoice choice;

    if (flStudioProjectFile.hasFileExtension (".flp")
        && flStudioProjectFile.existsAsFile())
    {
        choice.folder = flStudioProjectFile.getParentDirectory();
        choice.flStudioProjectFile = flStudioProjectFile;
        choice.sourceDescription = "FL Studio project: " + flStudioProjectFile.getFileName();
        choice.fromFlStudioProject = true;
        choice.folder.createDirectory();
        return choice;
    }

    choice.folder = fallbackDownloadsDirectory;
    choice.sourceDescription = "Downloads";
    choice.folder.createDirectory();
    return choice;
}

DownloadFolderChoice getDownloadFolderChoice()
{
    return chooseDownloadFolder (findRecentFlStudioProjectFile(),
                                 getFallbackDownloadsDirectory());
}

juce::File getDownloadDirectory()
{
    return getDownloadFolderChoice().folder;
}

TimecodeParseResult parseTimecodeToSeconds (juce::String text)
{
    TimecodeParseResult result;
    text = text.trim();

    if (text.isEmpty())
        return result;

    juce::StringArray parts;
    parts.addTokens (text, ":", "");
    parts.trim();
    parts.removeEmptyStrings();

    if (parts.isEmpty() || parts.size() > 3)
        return result;

    double parsed[3] = {};

    for (int i = 0; i < parts.size(); ++i)
        if (! parseNonNegativeNumber (parts[i], parsed[i]))
            return result;

    if (parts.size() == 1)
    {
        result.seconds = parsed[0];
    }
    else if (parts.size() == 2)
    {
        if (parsed[1] >= 60.0)
            return result;

        result.seconds = parsed[0] * 60.0 + parsed[1];
    }
    else
    {
        if (parsed[1] >= 60.0 || parsed[2] >= 60.0)
            return result;

        result.seconds = parsed[0] * 3600.0 + parsed[1] * 60.0 + parsed[2];
    }

    result.valid = true;
    return result;
}

DownloadSectionValidation validateDownloadSection (bool enabled,
                                                   const juce::String& startTime,
                                                   const juce::String& endTime)
{
    DownloadSectionValidation validation;
    validation.section.enabled = false;

    if (! enabled)
        return validation;

    const auto start = startTime.trim();
    const auto end = endTime.trim();

    if (start.isEmpty() || end.isEmpty())
    {
        validation.valid = false;
        validation.message = "Enter both clip start and clip end.";
        return validation;
    }

    const auto parsedStart = parseTimecodeToSeconds (start);
    const auto parsedEnd = parseTimecodeToSeconds (end);

    if (! parsedStart.valid || ! parsedEnd.valid)
    {
        validation.valid = false;
        validation.message = "Use seconds, mm:ss, or hh:mm:ss for clip times.";
        return validation;
    }

    if (parsedEnd.seconds <= parsedStart.seconds)
    {
        validation.valid = false;
        validation.message = "Clip end must be after clip start.";
        return validation;
    }

    validation.section.enabled = true;
    validation.section.startTime = start;
    validation.section.endTime = end;
    return validation;
}

juce::StringArray buildYtDlpCommand (const juce::String& url,
                                     const juce::File& downloadFolder,
                                     const DownloadOptions& options,
                                     const juce::File& toolDirectory)
{
    juce::StringArray command;
    const auto bundledYtDlp = findBundledExecutable ("yt-dlp", toolDirectory);
    const auto deno = findBundledExecutable ("deno", toolDirectory);
    const auto usingBundledYtDlp = bundledYtDlp.existsAsFile();

    if (usingBundledYtDlp)
    {
        command.add (bundledYtDlp.getFullPathName());
    }
    else
    {
        command.add (resolveExecutableForChildProcess ("uvx", toolDirectory));
        command.add ("--refresh-package");
        command.add ("yt-dlp");
        command.add ("--from");
        command.add ("yt-dlp@latest");
        command.add ("yt-dlp");
    }

    if (usingBundledYtDlp && deno.existsAsFile())
    {
        command.add ("--js-runtimes");
        command.add ("deno:" + deno.getFullPathName());
        command.add ("--remote-components");
        command.add ("ejs:npm");
    }

    command.add ("-f");
    command.add (options.section.enabled
                    ? "best[protocol*=m3u8][height<=360]/best[protocol*=m3u8]/bestaudio"
                    : "bestaudio");
    command.add ("-x");
    command.add ("--audio-format");
    command.add ("wav");
    command.add ("--no-playlist");
    command.add ("--force-overwrites");
    command.add ("--newline");
    command.add ("--print");
    command.add ("after_move:filepath");

    if (options.section.enabled)
    {
        command.add ("--download-sections");
        command.add ("*" + options.section.startTime.trim()
                     + "-" + options.section.endTime.trim());
    }

    if (const auto ffmpeg = findBundledExecutable ("ffmpeg", toolDirectory); ffmpeg.existsAsFile())
    {
        command.add ("--ffmpeg-location");
        command.add (ffmpeg.getParentDirectory().getFullPathName());
    }

    command.add ("--output");
    command.add (downloadFolder.getChildFile (getOutputTemplateName (options)).getFullPathName());
    command.add (url.trim());

    return command;
}

juce::StringArray buildYtDlpCommand (const juce::String& url,
                                     const juce::File& downloadFolder,
                                     const juce::File& toolDirectory)
{
    return buildYtDlpCommand (url, downloadFolder, {}, toolDirectory);
}

YtDlpOutputAnalysis analyseYtDlpOutput (const juce::String& output,
                                        const juce::File& downloadFolder)
{
    YtDlpOutputAnalysis analysis;
    auto lines = juce::StringArray::fromLines (output);

    for (auto line : lines)
    {
        line = line.trim();

        if (line.containsIgnoreCase ("ERROR:"))
        {
            analysis.hasError = true;
            analysis.message = line;
            return analysis;
        }

        if (! analysis.downloadedFile.existsAsFile())
        {
            if (const auto file = fileFromOutputLine (line, downloadFolder);
                file != juce::File())
            {
                analysis.downloadedFile = file;
            }
        }
    }

    return analysis;
}

juce::File findNewestSupportedAudioFile (const juce::File& downloadFolder)
{
    juce::Array<juce::File> files;
    downloadFolder.findChildFiles (files, juce::File::findFiles, false);

    juce::File newest;
    juce::Time newestTime;

    for (const auto& file : files)
    {
        if (! file.hasFileExtension (supportedAudioExtensions))
            continue;

        const auto modificationTime = file.getLastModificationTime();

        if (newest == juce::File() || modificationTime > newestTime)
        {
            newest = file;
            newestTime = modificationTime;
        }
    }

    return newest;
}

DownloadJobResult downloadAudioWithYtDlp (const juce::String& url,
                                          const juce::File& downloadFolder,
                                          const DownloadOptions& options)
{
    DownloadJobResult result;

    if (! isLikelySupportedUrl (url))
    {
        result.message = "Enter a valid http or https URL.";
        return result;
    }

    if (downloadFolder.createDirectory().failed())
    {
        result.message = "Could not create download folder: " + downloadFolder.getFullPathName();
        return result;
    }

    juce::ChildProcess process;
    const auto command = buildYtDlpCommand (url, downloadFolder, options);

    if (! process.start (command))
    {
        result.message = "Could not start yt-dlp. Run the StashTrack installer so yt-dlp.exe and its helper tools are copied next to the plug-in.";
        return result;
    }

    result.processOutput = process.readAllProcessOutput();
    result.exitCode = process.getExitCode();

    const auto analysis = analyseYtDlpOutput (result.processOutput, downloadFolder);

    if (analysis.hasError)
    {
        result.message = analysis.message;
        return result;
    }

    if (result.exitCode != 0)
    {
        result.message = "yt-dlp exited with code " + juce::String (result.exitCode)
                       + ". " + summariseOutput (result.processOutput);
        return result;
    }

    auto downloaded = analysis.downloadedFile;

    if (! downloaded.existsAsFile())
        downloaded = findNewestSupportedAudioFile (downloadFolder);

    if (! downloaded.existsAsFile())
    {
        result.message = "Download finished, but no supported audio file was found in "
                       + downloadFolder.getFullPathName() + ". "
                       + summariseOutput (result.processOutput);
        return result;
    }

    result.succeeded = true;
    result.downloadedFile = downloaded;
    result.message = "Downloaded " + downloaded.getFileName();
    return result;
}

DownloadJobResult downloadAudioWithYtDlp (const juce::String& url,
                                          const juce::File& downloadFolder)
{
    return downloadAudioWithYtDlp (url, downloadFolder, {});
}
}
