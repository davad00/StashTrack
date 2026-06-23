#include "PluginEditor.h"

#if STASHTRACK_HAS_REACT_JUCE
 #include <react_juce.h>
#endif

namespace
{
    const juce::Colour kBackground { 0xff0a0b0a };
    const juce::Colour kPanel      { 0xff101210 };
    const juce::Colour kPanelLift  { 0xff14170f };
    const juce::Colour kWell       { 0xff0c0e0c };
    const juce::Colour kLine       { 0xff2b3029 };
    const juce::Colour kAccent     { 0xffc6f135 };
    const juce::Colour kText       { 0xffe8eae6 };
    const juce::Colour kMuted      { 0xff9aa097 };
    const juce::Colour kError      { 0xffff4f64 };

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

#if STASHTRACK_HAS_REACT_JUCE
class ReactJuceBackdropComponent final : public juce::Component
{
public:
    ReactJuceBackdropComponent()
        : engine (std::make_shared<reactjuce::EcmascriptEngine>()),
          appRoot (engine)
    {
        addAndMakeVisible (appRoot);

        const juce::File bundle { juce::String (STASHTRACK_REACT_JUCE_BUNDLE_PATH) };

        if (! bundle.existsAsFile())
            return;

        try
        {
            appRoot.reset();
            appRoot.bindNativeRenderingHooks();
            appRoot.evaluate (bundle);
            bundleLoaded = true;
        }
        catch (const std::exception& e)
        {
            juce::ignoreUnused (e);
            DBG ("React-JUCE bundle failed: " << e.what());
            appRoot.setVisible (false);
        }
    }

    void resized() override
    {
        appRoot.setBounds (getLocalBounds());
    }

    bool isBundleLoaded() const noexcept
    {
        return bundleLoaded;
    }

private:
    std::shared_ptr<reactjuce::EcmascriptEngine> engine;
    reactjuce::ReactApplicationRoot appRoot;
    bool bundleLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReactJuceBackdropComponent)
};
#else
class ReactJuceBackdropComponent final : public juce::Component
{
public:
    bool isBundleLoaded() const noexcept { return false; }
};
#endif

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

    reactBackdrop = std::make_unique<ReactJuceBackdropComponent>();

    if (reactBackdrop->isBundleLoaded())
    {
        addAndMakeVisible (*reactBackdrop);
        reactBackdrop->toBack();
    }
    else
    {
        reactBackdrop.reset();
    }

    titleLabel.setText ("StashTrack v" + juce::String (JucePlugin_VersionString),
                         juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions { 24.0f, juce::Font::bold });
    titleLabel.setColour (juce::Label::textColourId, kText);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    urlLabel.setText ("SOURCE URL", juce::dontSendNotification);
    urlLabel.setFont (juce::FontOptions { 12.0f });
    urlLabel.setColour (juce::Label::textColourId, kMuted);
    addAndMakeVisible (urlLabel);

    urlField.setMultiLine (false);
    urlField.setTextToShowWhenEmpty ("https://www.youtube.com/watch?v=...", kMuted);
    urlField.setColour (juce::TextEditor::backgroundColourId, kWell);
    urlField.setColour (juce::TextEditor::textColourId, kText);
    urlField.setColour (juce::TextEditor::outlineColourId, kLine);
    urlField.setColour (juce::TextEditor::focusedOutlineColourId, kAccent);
    urlField.setColour (juce::TextEditor::highlightColourId, kAccent.withAlpha (0.35f));
    urlField.setFont (juce::FontOptions { 15.0f });
    addAndMakeVisible (urlField);

    downloadButton.setColour (juce::TextButton::buttonColourId, kAccent);
    downloadButton.setColour (juce::TextButton::buttonOnColourId, kAccent.darker (0.08f));
    downloadButton.setColour (juce::TextButton::textColourOnId, kBackground);
    downloadButton.setColour (juce::TextButton::textColourOffId, kBackground);
    downloadButton.onClick = [this] { startDownload(); };
    addAndMakeVisible (downloadButton);

    clipToggle.setColour (juce::ToggleButton::textColourId, kText);
    clipToggle.setColour (juce::ToggleButton::tickColourId, kAccent);
    clipToggle.setColour (juce::ToggleButton::tickDisabledColourId, kMuted);
    clipToggle.onClick = [this] { updateClipControls(); };
    addAndMakeVisible (clipToggle);

    startLabel.setText ("START", juce::dontSendNotification);
    startLabel.setFont (juce::FontOptions { 12.0f, juce::Font::bold });
    startLabel.setColour (juce::Label::textColourId, kMuted);
    startLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (startLabel);

    endLabel.setText ("END", juce::dontSendNotification);
    endLabel.setFont (juce::FontOptions { 12.0f, juce::Font::bold });
    endLabel.setColour (juce::Label::textColourId, kMuted);
    endLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (endLabel);

    for (auto* field : { &startField, &endField })
    {
        field->setMultiLine (false);
        field->setColour (juce::TextEditor::backgroundColourId, kWell);
        field->setColour (juce::TextEditor::textColourId, kText);
        field->setColour (juce::TextEditor::outlineColourId, kLine);
        field->setColour (juce::TextEditor::focusedOutlineColourId, kAccent);
        field->setColour (juce::TextEditor::highlightColourId, kAccent.withAlpha (0.35f));
        field->setFont (juce::FontOptions { 14.0f });
        addAndMakeVisible (*field);
    }

    startField.setTextToShowWhenEmpty ("0:30", kMuted);
    endField.setTextToShowWhenEmpty ("1:00", kMuted);

    waveform = std::make_unique<WaveformFileDragComponent>();
    waveform->onExternalDragStarted = [this] (bool started)
    {
        setStatus (started ? "Drop into an empty FL playlist track."
                           : "Could not start the host file drag.",
                   started ? kAccent : kError);
    };
    addAndMakeVisible (*waveform);

    statusLabel.setText (describeChoice (currentDownloadChoice),
                         juce::dontSendNotification);
    statusLabel.setFont (juce::FontOptions { 12.0f });
    statusLabel.setColour (juce::Label::textColourId, kMuted);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    updateClipControls();

    setSize (720, 430);
}

YouTubeGrabberAudioProcessorEditor::~YouTubeGrabberAudioProcessorEditor()
{
    stopThread (4000);
}

//==============================================================================
void YouTubeGrabberAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBackground);

    g.setColour (kLine.withAlpha (0.45f));
    for (int x = 0; x < getWidth(); x += 88)
        g.drawVerticalLine (x, 0.0f, static_cast<float> (getHeight()));

    for (int y = 0; y < getHeight(); y += 88)
        g.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));

    auto panel = getLocalBounds().reduced (14).toFloat();
    g.setColour (kPanel);
    g.fillRect (panel);
    g.setColour (kLine);
    g.drawRect (panel.reduced (0.5f), 1.0f);

    const auto header = panel.withHeight (52.0f);
    g.setColour (kWell);
    g.fillRect (header);
    g.setColour (kLine);
    g.drawLine (panel.getX(), header.getBottom(), panel.getRight(), header.getBottom(), 1.0f);

    const auto markX = panel.getX() + 22.0f;
    const auto markY = panel.getY() + 19.0f;
    g.setColour (kAccent);
    g.fillRect (juce::Rectangle<float> (markX, markY, 10.0f, 14.0f));
    g.fillRect (juce::Rectangle<float> (markX + 16.0f, markY + 4.0f, 10.0f, 10.0f));
    g.fillRect (juce::Rectangle<float> (markX + 32.0f, markY - 4.0f, 10.0f, 18.0f));

    auto body = panel.reduced (18.0f).withTrimmedTop (64.0f).withTrimmedBottom (52.0f);
    g.setColour (kWell.withAlpha (0.72f));
    g.fillRect (body);
    g.setColour (kLine.withAlpha (0.8f));
    g.drawRect (body.reduced (0.5f), 1.0f);
}

void YouTubeGrabberAudioProcessorEditor::resized()
{
    if (reactBackdrop != nullptr)
        reactBackdrop->setBounds (getLocalBounds());

    auto area = getLocalBounds().reduced (14).reduced (22, 18);

    auto header = area.removeFromTop (34);
    header.removeFromLeft (58);
    titleLabel.setBounds (header.removeFromLeft (230));
    statusLabel.setBounds (header);
    area.removeFromTop (16);

    urlLabel.setBounds (area.removeFromTop (18));
    auto urlRow = area.removeFromTop (42);
    downloadButton.setBounds (urlRow.removeFromRight (124));
    urlRow.removeFromRight (12);
    urlField.setBounds (urlRow);

    area.removeFromTop (12);
    auto clipRow = area.removeFromTop (42);
    clipToggle.setBounds (clipRow.removeFromLeft (78));
    clipRow.removeFromLeft (12);
    startLabel.setBounds (clipRow.removeFromLeft (42));
    startField.setBounds (clipRow.removeFromLeft (116));
    clipRow.removeFromLeft (12);
    endLabel.setBounds (clipRow.removeFromLeft (30));
    endField.setBounds (clipRow.removeFromLeft (116));

    area.removeFromTop (14);

    waveform->setBounds (area);
}

//==============================================================================
void YouTubeGrabberAudioProcessorEditor::startDownload()
{
    if (isThreadRunning())
    {
        setStatus ("A download is already running.", kMuted);
        return;
    }

    pendingUrl = urlField.getText().trim();
    pendingDownloadChoice = StashTrack::getDownloadFolderChoice();
    pendingDownloadOptions = {};

    if (! StashTrack::isLikelySupportedUrl (pendingUrl))
    {
        const auto message = pendingUrl.isEmpty()
                           ? juce::String ("Please paste a URL first.")
                           : juce::String ("Enter a valid http or https URL.");

        setStatus (message, kError);
        showErrorAlert ("Invalid URL", message);
        return;
    }

    const auto sectionValidation = StashTrack::validateDownloadSection (clipToggle.getToggleState(),
                                                                        startField.getText(),
                                                                        endField.getText());

    if (! sectionValidation.valid)
    {
        setStatus (sectionValidation.message, kError);
        showErrorAlert ("Invalid clip range", sectionValidation.message);
        return;
    }

    pendingDownloadOptions.section = sectionValidation.section;

    downloadedFile = {};
    waveform->setAudioFile ({});
    downloadButton.setEnabled (false);
    clipToggle.setEnabled (false);
    startField.setEnabled (false);
    endField.setEnabled (false);

    setStatus ((pendingDownloadOptions.section.enabled ? "Downloading clip to " : "Downloading to ")
               + describeChoice (pendingDownloadChoice),
               kMuted);

    startThread();
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
    downloadButton.setEnabled (true);
    clipToggle.setEnabled (true);
    updateClipControls();

    if (result.succeeded)
    {
        currentDownloadChoice = pendingDownloadChoice;
        downloadedFile = result.downloadedFile;
        waveform->setAudioFile (downloadedFile);
        setStatus ((pendingDownloadOptions.section.enabled ? "Clip ready: " : "Ready: ")
                   + downloadedFile.getFileName(),
                   kAccent);
        return;
    }

    waveform->setAudioFile ({});
    const auto message = result.message.isNotEmpty()
                       ? result.message
                       : juce::String ("Download failed. Check uv/uvx and ffmpeg.");
    setStatus (message, kError);
    showErrorAlert ("Download failed", message);
}

void YouTubeGrabberAudioProcessorEditor::updateClipControls()
{
    const auto enabled = clipToggle.getToggleState() && clipToggle.isEnabled();

    startLabel.setEnabled (enabled);
    endLabel.setEnabled (enabled);
    startField.setEnabled (enabled);
    endField.setEnabled (enabled);
    startField.setAlpha (enabled ? 1.0f : 0.45f);
    endField.setAlpha (enabled ? 1.0f : 0.45f);
}

void YouTubeGrabberAudioProcessorEditor::setStatus (const juce::String& message,
                                                    juce::Colour colour)
{
    statusLabel.setColour (juce::Label::textColourId, colour);
    statusLabel.setText (message, juce::dontSendNotification);
}

void YouTubeGrabberAudioProcessorEditor::showErrorAlert (const juce::String& title,
                                                         const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                            title,
                                            message);
}
