#include "UpdateUtils.h"
#include "DownloadUtils.h"

namespace StashTrack
{
namespace
{
    constexpr auto latestReleaseApiUrl = "https://api.github.com/repos/davad00/StashTrack/releases/latest";
    constexpr auto githubHeaders = "Accept: application/vnd.github+json\r\n"
                                   "User-Agent: StashTrack-Updater\r\n";

    juce::Array<int> parseVersionParts (const juce::String& version)
    {
        juce::Array<int> parts;
        juce::StringArray tokens;
        tokens.addTokens (normaliseVersionTag (version), ".", "");
        tokens.trim();
        tokens.removeEmptyStrings();

        for (auto token : tokens)
        {
            token = token.retainCharacters ("0123456789");

            if (token.isEmpty())
                parts.add (0);
            else
                parts.add (token.getIntValue());
        }

        while (parts.size() < 3)
            parts.add (0);

        return parts;
    }

    bool assetLooksLikeWindowsInstaller (const juce::String& name)
    {
        return name.matchesWildcard ("StashTrackv*Setup.exe", true);
    }

    std::unique_ptr<juce::InputStream> createGithubInputStream (const juce::String& url,
                                                                int& statusCode,
                                                                const juce::String& extraHeaders = githubHeaders)
    {
        return juce::URL (url).createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs (15000)
                .withNumRedirectsToFollow (5)
                .withExtraHeaders (extraHeaders)
                .withStatusCode (&statusCode));
    }

    juce::String downloadText (const juce::String& url)
    {
        int statusCode = 0;
        auto stream = createGithubInputStream (url, statusCode);

        if (stream == nullptr || statusCode < 200 || statusCode >= 300)
            return {};

        return stream->readEntireStreamAsString();
    }
}

juce::String normaliseVersionTag (const juce::String& version)
{
    const auto trimmed = version.trim();

    for (int i = 0; i < trimmed.length(); ++i)
        if (juce::CharacterFunctions::isDigit (trimmed[i]))
            return trimmed.substring (i);

    return trimmed;
}

bool isVersionNewer (const juce::String& currentVersion,
                     const juce::String& candidateVersion)
{
    const auto current = parseVersionParts (currentVersion);
    const auto candidate = parseVersionParts (candidateVersion);
    const auto count = juce::jmax (current.size(), candidate.size());

    for (int i = 0; i < count; ++i)
    {
        const auto a = i < current.size() ? current[i] : 0;
        const auto b = i < candidate.size() ? candidate[i] : 0;

        if (b > a)
            return true;

        if (b < a)
            return false;
    }

    return false;
}

juce::String getReleaseChangelogUrl (const LatestReleaseInfo& release)
{
    const auto url = release.releasePageUrl.trim();
    return url.startsWithIgnoreCase ("https://") ? url : juce::String();
}

LatestReleaseInfo parseLatestReleaseJson (const juce::String& jsonText)
{
    LatestReleaseInfo info;
    const auto parsed = juce::JSON::parse (jsonText);
    const auto* release = parsed.getDynamicObject();

    if (release == nullptr)
        return info;

    info.versionTag = release->getProperty ("tag_name").toString().trim();
    info.releasePageUrl = release->getProperty ("html_url").toString().trim();

    if (auto* assets = release->getProperty ("assets").getArray())
    {
        for (const auto& assetVar : *assets)
        {
            const auto* asset = assetVar.getDynamicObject();

            if (asset == nullptr)
                continue;

            const auto name = asset->getProperty ("name").toString().trim();

            if (! assetLooksLikeWindowsInstaller (name))
                continue;

            info.installerUrl = asset->getProperty ("browser_download_url").toString().trim();
            break;
        }
    }

    info.valid = info.versionTag.isNotEmpty()
              && info.installerUrl.startsWithIgnoreCase ("https://");

    return info;
}

LatestReleaseInfo fetchLatestReleaseInfo()
{
    return parseLatestReleaseJson (downloadText (latestReleaseApiUrl));
}

UpdateCheckResult checkForUpdate (const juce::String& currentVersion)
{
    UpdateCheckResult result;
    result.latest = fetchLatestReleaseInfo();

    if (! result.latest.valid)
    {
        result.message = "Could not check for updates.";
        return result;
    }

    result.succeeded = true;
    result.updateAvailable = isVersionNewer (currentVersion, result.latest.versionTag);
    result.message = result.updateAvailable
                   ? "StashTrack " + result.latest.versionTag + " is available."
                   : "StashTrack is up to date.";
    return result;
}

juce::File getUpdaterDownloadFile (const juce::String& versionTag,
                                   const juce::File& downloadFolder)
{
    const auto folder = downloadFolder == juce::File()
                      ? getFallbackDownloadsDirectory()
                      : downloadFolder;
    const auto tag = versionTag.trim().isNotEmpty() ? versionTag.trim() : juce::String ("latest");

    return folder.getChildFile ("StashTrack" + tag + "Setup.exe");
}

UpdateInstallResult downloadInstallerToFile (const juce::String& installerUrl,
                                             const juce::File& destinationFile)
{
    UpdateInstallResult result;

    if (! installerUrl.startsWithIgnoreCase ("https://"))
    {
        result.message = "Update installer URL is invalid.";
        return result;
    }

    if (destinationFile.getParentDirectory().createDirectory().failed())
    {
        result.message = "Could not create update download folder.";
        return result;
    }

    int statusCode = 0;
    auto input = createGithubInputStream (installerUrl,
                                          statusCode,
                                          "Accept: application/octet-stream\r\n"
                                          "User-Agent: StashTrack-Updater\r\n");

    if (input == nullptr || statusCode < 200 || statusCode >= 300)
    {
        result.message = "Could not download update installer.";
        return result;
    }

    destinationFile.deleteFile();

    if (auto output = destinationFile.createOutputStream())
    {
        output->writeFromInputStream (*input, -1);
        output->flush();
    }

    if (! destinationFile.existsAsFile() || destinationFile.getSize() <= 0)
    {
        result.message = "Update installer download did not create a valid file.";
        return result;
    }

    result.succeeded = true;
    result.installerFile = destinationFile;
    result.message = "Downloaded " + destinationFile.getFileName();
    return result;
}

bool launchInstaller (const juce::File& installerFile)
{
    if (! installerFile.existsAsFile())
        return false;

    return juce::Process::openDocument (installerFile.getFullPathName(), {});
}
}
