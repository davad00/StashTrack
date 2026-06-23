#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

//==============================================================================
/**
    StashTrack downloads audio from a YouTube or yt-dlp-supported link and
    prepares it for host import.

    The heavy lifting (the actual download) is handled by the editor via a
    background thread. The editor now exposes the result as a native draggable
    audio file, so the default audio path stays silent unless a future workflow
    explicitly calls loadAudioFile().
*/
class YouTubeGrabberAudioProcessor : public juce::AudioProcessor
{
public:
    YouTubeGrabberAudioProcessor();
    ~YouTubeGrabberAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Called by the editor once a file has been downloaded.
    // Returns true if the file was decoded and loaded successfully.
    bool loadAudioFile (const juce::File& file);

    // Whether a sample is currently loaded and ready to play.
    bool hasSample() const noexcept { return hasLoadedAudio; }

private:
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    juce::TimeSliceThread readAheadThread { "StashTrack audio read-ahead" };

    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

    std::atomic<bool> hasLoadedAudio { false };
    juce::CriticalSection transportLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YouTubeGrabberAudioProcessor)
};
