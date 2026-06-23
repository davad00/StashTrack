#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
YouTubeGrabberAudioProcessor::YouTubeGrabberAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();
    readAheadThread.startThread();
}

YouTubeGrabberAudioProcessor::~YouTubeGrabberAudioProcessor()
{
    const juce::ScopedLock sl (transportLock);
    transportSource.stop();
    transportSource.setSource (nullptr);
    readerSource.reset();
    readAheadThread.stopThread (1000);
}

//==============================================================================
void YouTubeGrabberAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::ScopedLock sl (transportLock);
    transportSource.prepareToPlay (samplesPerBlock, sampleRate);
}

void YouTubeGrabberAudioProcessor::releaseResources()
{
    const juce::ScopedLock sl (transportLock);
    transportSource.releaseResources();
}

bool YouTubeGrabberAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void YouTubeGrabberAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    if (! hasLoadedAudio.load())
        return;

    const juce::ScopedLock sl (transportLock);
    juce::AudioSourceChannelInfo info (&buffer, 0, buffer.getNumSamples());
    transportSource.getNextAudioBlock (info);
}

//==============================================================================
bool YouTubeGrabberAudioProcessor::loadAudioFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr)
        return false;

    const auto sourceSampleRate = reader->sampleRate;
    auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader.release(), true);

    {
        const juce::ScopedLock sl (transportLock);

        transportSource.stop();
        transportSource.setSource (nullptr);
        readerSource = std::move (newSource);
        transportSource.setSource (readerSource.get(), 32768, &readAheadThread, sourceSampleRate);
        transportSource.setPosition (0.0);
        transportSource.start();
    }

    hasLoadedAudio = true;
    return true;
}

//==============================================================================
juce::AudioProcessorEditor* YouTubeGrabberAudioProcessor::createEditor()
{
    return new YouTubeGrabberAudioProcessorEditor (*this);
}

//==============================================================================
void YouTubeGrabberAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void YouTubeGrabberAudioProcessor::setStateInformation (const void*, int) {}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new YouTubeGrabberAudioProcessor();
}
