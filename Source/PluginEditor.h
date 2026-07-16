#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <vsreact/vsreact.h>
#include "PluginProcessor.h"
#include "DownloadUtils.h"
#include "UpdateUtils.h"

#include <atomic>
#include <thread>

class WaveformFileDragComponent;

//==============================================================================
/**
    The plugin front panel.

    The entire UI is a React + TypeScript app (jsui-vsreact/) rendered by the
    VSReacT engine. C++ keeps the pieces that must stay native: the download
    and update worker threads, and the waveform component that performs the
    host file drag (registered as the "waveform" NativeView).
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
    juce::var handleNativeCall (const juce::String& name, const juce::var& args);
    juce::var startDownloadFromJs (const juce::var& args);
    void run() override;                 // background download thread
    void downloadFinished (StashTrack::DownloadJobResult result);
    void startUpdateCheck();
    void updateCheckFinished (StashTrack::UpdateCheckResult result);
    void startUpdateInstallerDownload (StashTrack::LatestReleaseInfo latest);
    void updateInstallerDownloadFinished (StashTrack::UpdateInstallResult result);
    void setStatus (const juce::String& message, const juce::String& tone);
    void sendDownloadState (bool running);
    void showErrorAlert (const juce::String& title, const juce::String& message);
    WaveformFileDragComponent* waveformComponent() const;

    //==============================================================================
    YouTubeGrabberAudioProcessor& processorRef;

    std::unique_ptr<vsreact::RootView> reactRoot;
    juce::Component::SafePointer<juce::Component> waveform;

    // Shared between the UI thread and the background download thread.
    juce::String pendingUrl;
    StashTrack::DownloadFolderChoice currentDownloadChoice;
    StashTrack::DownloadFolderChoice pendingDownloadChoice;
    StashTrack::DownloadOptions pendingDownloadOptions;
    juce::File downloadedFile;
    std::thread updateCheckThread;
    std::thread updateInstallerThread;
    std::atomic<bool> closing { false };
    bool updateInstallerDownloadRunning = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YouTubeGrabberAudioProcessorEditor)
};
