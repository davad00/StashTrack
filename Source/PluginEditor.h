#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "DownloadUtils.h"

class WaveformFileDragComponent;
class ReactJuceBackdropComponent;

//==============================================================================
/**
    The plugin front panel.

    A text field for a YouTube link, a "Download" button that pulls audio down in
    the background, and a waveform surface that starts a native file drag into
    the host playlist.
*/
class YouTubeGrabberAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Thread
{
public:
    explicit YouTubeGrabberAudioProcessorEditor (YouTubeGrabberAudioProcessor&);
    ~YouTubeGrabberAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    void startDownload();
    void run() override;                 // background download thread
    void downloadFinished (StashTrack::DownloadJobResult result);
    void updateClipControls();
    void setStatus (const juce::String& message, juce::Colour colour);
    void showErrorAlert (const juce::String& title, const juce::String& message);

    //==============================================================================
    YouTubeGrabberAudioProcessor& processorRef;

    juce::Label      titleLabel;
    juce::Label      urlLabel;
    juce::TextEditor urlField;
    juce::TextButton downloadButton { "Download" };
    juce::ToggleButton clipToggle { "Clip" };
    juce::Label      startLabel;
    juce::Label      endLabel;
    juce::TextEditor startField;
    juce::TextEditor endField;
    juce::Label      statusLabel;
    std::unique_ptr<ReactJuceBackdropComponent> reactBackdrop;
    std::unique_ptr<WaveformFileDragComponent> waveform;

    // Shared between the UI thread and the background download thread.
    juce::String pendingUrl;
    StashTrack::DownloadFolderChoice currentDownloadChoice;
    StashTrack::DownloadFolderChoice pendingDownloadChoice;
    StashTrack::DownloadOptions pendingDownloadOptions;
    juce::File downloadedFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YouTubeGrabberAudioProcessorEditor)
};
