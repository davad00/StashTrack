#include "PluginEditor.h"

#if ! STASHTRACK_VSREACT_DEV
 #include "BinaryData.h"
#endif

namespace
{
    const juce::Colour kBackground { 0xff0a0b0a };
    const juce::Colour kPanelLift  { 0xff14170f };
    const juce::Colour kWell       { 0xff0c0e0c };
    const juce::Colour kLine       { 0xff2b3029 };
    const juce::Colour kAccent     { 0xffc6f135 };
    const juce::Colour kText       { 0xffe8eae6 };
    const juce::Colour kMuted      { 0xff9aa097 };

    juce::String shortenPathForStatus (const juce::File& folder)
    {
        const auto path = folder.getFullPathName();

        if (path.length() <= 58)
            return path;

        return "..." + path.substring (path.length() - 55);
    }

    juce::String describeChoice (const StashTrack::DownloadFolderChoice& choice)
    {
        auto label = choice.fromFlStudioProject ? "Project" : "Downloads";

        return juce::String (label) + ": " + shortenPathForStatus (choice.folder);
    }

    juce::String formatDuration (double seconds)
    {
        if (seconds <= 0.0)
            return {};

        const auto totalSeconds = juce::roundToInt (seconds);
        const auto minutes = totalSeconds / 60;
        const auto remainder = totalSeconds % 60;

        return juce::String (minutes) + ":"
             + juce::String (remainder).paddedLeft ('0', 2);
    }
}

//==============================================================================
class WaveformFileDragComponent final : public juce::Component,
                                        private juce::ChangeListener
{
public:
    WaveformFileDragComponent()
        : thumbnail (512, formatManager, thumbnailCache)
    {
        formatManager.registerBasicFormats();
        thumbnail.addChangeListener (this);
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    ~WaveformFileDragComponent() override
    {
        thumbnail.removeChangeListener (this);
    }

    void setAudioFile (const juce::File& file)
    {
        audioFile = file;
        dragStarted = false;
        thumbnail.clear();

        if (audioFile.existsAsFile())
            thumbnail.setSource (new juce::FileInputSource (audioFile));

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        g.setColour (kPanelLift);
        g.fillRect (bounds);
        g.setColour (kLine);
        g.drawRect (bounds.reduced (0.5f), 1.0f);

        auto area = getLocalBounds().reduced (18, 14);
        auto header = area.removeFromTop (28);
        auto footer = area.removeFromBottom (24);
        area.removeFromTop (8);
        area.removeFromBottom (8);

        g.setFont (juce::FontOptions { 15.0f, juce::Font::bold });
        g.setColour (kText);
        g.drawFittedText (audioFile.existsAsFile() ? audioFile.getFileName() : "No audio loaded",
                          header.removeFromLeft (juce::jmax (80, header.getWidth() - 64)),
                          juce::Justification::centredLeft,
                          1);

        if (thumbnail.getTotalLength() > 0.0)
        {
            g.setFont (juce::FontOptions { 12.0f });
            g.setColour (kMuted);
            g.drawFittedText (formatDuration (thumbnail.getTotalLength()),
                              header,
                              juce::Justification::centredRight,
                              1);
        }

        const auto waveBounds = area.toFloat();
        g.setColour (kWell);
        g.fillRect (waveBounds);

        g.setColour (kLine.withAlpha (0.7f));
        for (int i = 1; i < 8; ++i)
        {
            const auto x = waveBounds.getX() + waveBounds.getWidth() * static_cast<float> (i) / 8.0f;
            g.drawVerticalLine (juce::roundToInt (x), waveBounds.getY() + 6.0f, waveBounds.getBottom() - 6.0f);
        }

        g.setColour (kLine.brighter (0.2f));
        g.drawHorizontalLine (juce::roundToInt (waveBounds.getCentreY()),
                              waveBounds.getX() + 8.0f,
                              waveBounds.getRight() - 8.0f);

        if (thumbnail.getTotalLength() > 0.0)
        {
            g.setColour (kAccent);
            thumbnail.drawChannels (g,
                                    area.reduced (8, 10),
                                    0.0,
                                    thumbnail.getTotalLength(),
                                    0.95f);
        }
        else
        {
            g.setFont (juce::FontOptions { 15.0f });
            g.setColour (kMuted);
            g.drawFittedText ("Waveform appears after download",
                              area,
                              juce::Justification::centred,
                              1);
        }

        g.setFont (juce::FontOptions { 12.0f, juce::Font::bold });
        g.setColour (audioFile.existsAsFile() ? kAccent : kMuted);
        g.drawFittedText (audioFile.existsAsFile() ? "DRAG TO PLAYLIST" : "WAITING",
                          footer,
                          juce::Justification::centredLeft,
                          1);
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        if (! audioFile.existsAsFile() || dragStarted || event.getDistanceFromDragStart() < 8)
            return;

        dragStarted = true;

        juce::StringArray files;
        files.add (audioFile.getFullPathName());

        juce::Component::SafePointer<WaveformFileDragComponent> safeThis (this);

        const auto started = juce::DragAndDropContainer::performExternalDragDropOfFiles (
            files,
            false,
            this,
            [safeThis]
            {
                if (safeThis != nullptr)
                    safeThis->dragStarted = false;
            });

        if (onExternalDragStarted != nullptr)
            onExternalDragStarted (started);

        if (! started)
            dragStarted = false;
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        dragStarted = false;
    }

    std::function<void (bool)> onExternalDragStarted;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        repaint();
    }

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 4 };
    juce::AudioThumbnail thumbnail;
    juce::File audioFile;
    bool dragStarted = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformFileDragComponent)
};

//==============================================================================
YouTubeGrabberAudioProcessorEditor::YouTubeGrabberAudioProcessorEditor (
        YouTubeGrabberAudioProcessor& p)
    : AudioProcessorEditor (&p),
      juce::Thread ("YouTubeDownload"),
      processorRef (p)
{
    currentDownloadChoice = StashTrack::getDownloadFolderChoice();

    vsreact::NativeRegistry registry;

    registry.registerFactory ("waveform", [this]() -> std::unique_ptr<juce::Component>
    {
        auto component = std::make_unique<WaveformFileDragComponent>();

        component->onExternalDragStarted = [this] (bool started)
        {
            setStatus (started ? "Drop into an empty FL playlist track."
                               : "Could not start the host file drag.",
                       started ? "accent" : "error");
        };

        component->setAudioFile (downloadedFile);
        waveform = component.get();
        return component;
    });

    vsreact::RootOptions options;

#if STASHTRACK_VSREACT_DEV
    // Development: load the bundle from the source tree and hot-reload it.
    options.bundleFile = juce::File (juce::String (STASHTRACK_VSREACT_BUNDLE_PATH));
    options.watchForChanges = true;
#else
    // Production: the bundle is embedded in the binary.
    options.bundleSource = juce::String::fromUTF8 (BinaryData::main_js, BinaryData::main_jsSize);
#endif

    options.onNativeCall = [this] (const juce::String& name, const juce::var& args)
    {
        return handleNativeCall (name, args);
    };

    reactRoot = std::make_unique<vsreact::RootView> (std::move (options), std::move (registry));
    addAndMakeVisible (*reactRoot);

    setSize (720, 430);
    startUpdateCheck();
}

YouTubeGrabberAudioProcessorEditor::~YouTubeGrabberAudioProcessorEditor()
{
    closing = true;
    stopThread (4000);

    if (updateCheckThread.joinable())
        updateCheckThread.join();

    if (updateInstallerThread.joinable())
        updateInstallerThread.join();
}

//==============================================================================
void YouTubeGrabberAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBackground);
}

void YouTubeGrabberAudioProcessorEditor::resized()
{
    reactRoot->setBounds (getLocalBounds());
}

//==============================================================================
WaveformFileDragComponent* YouTubeGrabberAudioProcessorEditor::waveformComponent() const
{
    return static_cast<WaveformFileDragComponent*> (waveform.getComponent());
}

juce::var YouTubeGrabberAudioProcessorEditor::handleNativeCall (const juce::String& name,
                                                                const juce::var& args)
{
    if (name == "getInitialState")
    {
        auto* state = new juce::DynamicObject();
        state->setProperty ("version", JucePlugin_VersionString);
        state->setProperty ("choice", describeChoice (currentDownloadChoice));
        return juce::var (state);
    }

    if (name == "startDownload")
        return startDownloadFromJs (args);

    jassertfalse;   // unknown native call from JS
    return {};
}

juce::var YouTubeGrabberAudioProcessorEditor::startDownloadFromJs (const juce::var& args)
{
    const auto failure = [this] (const juce::String& title, const juce::String& message)
    {
        setStatus (message, "error");
        showErrorAlert (title, message);

        auto* result = new juce::DynamicObject();
        result->setProperty ("ok", false);
        result->setProperty ("message", message);
        return juce::var (result);
    };

    if (isThreadRunning())
    {
        setStatus ("A download is already running.", "muted");

        auto* result = new juce::DynamicObject();
        result->setProperty ("ok", false);
        return juce::var (result);
    }

    pendingUrl = args["url"].toString().trim();
    pendingDownloadChoice = StashTrack::getDownloadFolderChoice();
    pendingDownloadOptions = {};

    if (! StashTrack::isLikelySupportedUrl (pendingUrl))
    {
        const auto message = pendingUrl.isEmpty()
                           ? juce::String ("Please paste a URL first.")
                           : juce::String ("Enter a valid http or https URL.");

        return failure ("Invalid URL", message);
    }

    const auto sectionValidation = StashTrack::validateDownloadSection (
        static_cast<bool> (args["clip"]),
        args["start"].toString(),
        args["end"].toString());

    if (! sectionValidation.valid)
        return failure ("Invalid clip range", sectionValidation.message);

    pendingDownloadOptions.section = sectionValidation.section;

    downloadedFile = {};

    if (auto* component = waveformComponent())
        component->setAudioFile ({});

    sendDownloadState (true);
    setStatus ((pendingDownloadOptions.section.enabled ? "Downloading clip to " : "Downloading to ")
               + describeChoice (pendingDownloadChoice),
               "muted");

    startThread();

    auto* result = new juce::DynamicObject();
    result->setProperty ("ok", true);
    return juce::var (result);
}

void YouTubeGrabberAudioProcessorEditor::run()
{
    // yt-dlp/ffmpeg are the same style of command-line tools used by Stacher7.
    // They may download copyrighted or terms-restricted media, so only use this
    // for content you have the rights to use.
    const auto url = pendingUrl;
    const auto downloadFolder = pendingDownloadChoice.folder;
    const auto options = pendingDownloadOptions;
    auto result = StashTrack::downloadAudioWithYtDlp (url, downloadFolder, options);

    juce::Component::SafePointer<YouTubeGrabberAudioProcessorEditor> safeThis (this);

    juce::MessageManager::callAsync ([safeThis, result]() mutable
    {
        if (safeThis != nullptr)
            safeThis->downloadFinished (std::move (result));
    });
}

void YouTubeGrabberAudioProcessorEditor::downloadFinished (StashTrack::DownloadJobResult result)
{
    sendDownloadState (false);

    if (result.succeeded)
    {
        currentDownloadChoice = pendingDownloadChoice;
        downloadedFile = result.downloadedFile;

        if (auto* component = waveformComponent())
            component->setAudioFile (downloadedFile);

        setStatus ((pendingDownloadOptions.section.enabled ? "Clip ready: " : "Ready: ")
                   + downloadedFile.getFileName(),
                   "accent");
        return;
    }

    if (auto* component = waveformComponent())
        component->setAudioFile ({});

    const auto message = result.message.isNotEmpty()
                       ? result.message
                       : juce::String ("Download failed. Check uv/uvx and ffmpeg.");
    setStatus (message, "error");
    showErrorAlert ("Download failed", message);
}

//==============================================================================
void YouTubeGrabberAudioProcessorEditor::startUpdateCheck()
{
    if (updateCheckThread.joinable())
        return;

    juce::Component::SafePointer<YouTubeGrabberAudioProcessorEditor> safeThis (this);

    updateCheckThread = std::thread ([safeThis]
    {
        const auto result = StashTrack::checkForUpdate (JucePlugin_VersionString);

        juce::MessageManager::callAsync ([safeThis, result]() mutable
        {
            if (safeThis != nullptr)
                safeThis->updateCheckFinished (std::move (result));
        });
    });
}

void YouTubeGrabberAudioProcessorEditor::updateCheckFinished (StashTrack::UpdateCheckResult result)
{
    if (closing || ! result.succeeded || ! result.updateAvailable)
        return;

    setStatus ("Update available: " + result.latest.versionTag, "accent");

    juce::Component::SafePointer<YouTubeGrabberAudioProcessorEditor> safeThis (this);
    const auto latest = result.latest;
    const auto message = "You are running StashTrack v"
                       + juce::String (JucePlugin_VersionString)
                       + ". " + latest.versionTag
                       + " is available.\n\nDownload and open the installer now? "
                         "Close FL Studio before finishing the installer so the loaded VST3 can be replaced.";

    const auto changelogUrl = StashTrack::getReleaseChangelogUrl (latest);
    auto* alert = new juce::AlertWindow ("StashTrack update available",
                                         message,
                                         juce::AlertWindow::InfoIcon);

    alert->addButton ("Download update", 1);

    if (changelogUrl.isNotEmpty())
        alert->addButton ("Changelog", 2);

    alert->addButton ("Later", 0);
    alert->setAlwaysOnTop (true);
    alert->enterModalState (
        true,
        juce::ModalCallbackFunction::create ([safeThis, latest, changelogUrl] (int resultCode)
        {
            if (safeThis == nullptr)
                return;

            if (resultCode == 1)
            {
                safeThis->startUpdateInstallerDownload (latest);
                return;
            }

            if (resultCode == 2 && changelogUrl.isNotEmpty())
            {
                juce::URL (changelogUrl).launchInDefaultBrowser();
                safeThis->setStatus ("Opened changelog for " + latest.versionTag, "muted");
            }
        }),
        true);
}

void YouTubeGrabberAudioProcessorEditor::startUpdateInstallerDownload (StashTrack::LatestReleaseInfo latest)
{
    if (updateInstallerDownloadRunning)
    {
        setStatus ("Update download is already running.", "muted");
        return;
    }

    if (updateInstallerThread.joinable())
        updateInstallerThread.join();

    updateInstallerDownloadRunning = true;
    setStatus ("Downloading " + latest.versionTag + " installer...", "muted");

    const auto destination = StashTrack::getUpdaterDownloadFile (latest.versionTag);
    const auto installerUrl = latest.installerUrl;
    juce::Component::SafePointer<YouTubeGrabberAudioProcessorEditor> safeThis (this);

    updateInstallerThread = std::thread ([safeThis, installerUrl, destination]
    {
        auto result = StashTrack::downloadInstallerToFile (installerUrl, destination);

        juce::MessageManager::callAsync ([safeThis, result]() mutable
        {
            if (safeThis != nullptr)
                safeThis->updateInstallerDownloadFinished (std::move (result));
        });
    });
}

void YouTubeGrabberAudioProcessorEditor::updateInstallerDownloadFinished (StashTrack::UpdateInstallResult result)
{
    updateInstallerDownloadRunning = false;

    if (closing)
        return;

    if (! result.succeeded)
    {
        const auto message = result.message.isNotEmpty()
                           ? result.message
                           : juce::String ("Could not download the update installer.");
        setStatus (message, "error");
        showErrorAlert ("Update failed", message);
        return;
    }

    if (StashTrack::launchInstaller (result.installerFile))
    {
        setStatus ("Installer opened. Close FL Studio to finish updating.", "accent");
        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::InfoIcon,
            "Installer opened",
            "The StashTrack installer has been downloaded and opened.\n\n"
            "Close FL Studio before completing setup, then reopen FL Studio and rescan plugins if needed.");
        return;
    }

    const auto message = "Downloaded the installer, but could not open it: "
                       + result.installerFile.getFullPathName();
    setStatus (message, "error");
    showErrorAlert ("Update downloaded", message);
}

//==============================================================================
void YouTubeGrabberAudioProcessorEditor::setStatus (const juce::String& message,
                                                    const juce::String& tone)
{
    if (closing || reactRoot == nullptr)
        return;

    auto* payload = new juce::DynamicObject();
    payload->setProperty ("message", message);
    payload->setProperty ("tone", tone);
    reactRoot->sendNativeEvent ("status", juce::var (payload));
}

void YouTubeGrabberAudioProcessorEditor::sendDownloadState (bool running)
{
    if (closing || reactRoot == nullptr)
        return;

    auto* payload = new juce::DynamicObject();
    payload->setProperty ("running", running);
    reactRoot->sendNativeEvent ("downloadState", juce::var (payload));
}

void YouTubeGrabberAudioProcessorEditor::showErrorAlert (const juce::String& title,
                                                         const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                            title,
                                            message);
}
