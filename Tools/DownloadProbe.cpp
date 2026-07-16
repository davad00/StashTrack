// Live smoke test for the download pipeline: runs a real yt-dlp download with
// the progress callback and prints the percent stream, so the determinate
// progress bar path is verified end to end.
//
//   StashTrackDownloadProbe <url> <downloadFolder>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include "../Source/DownloadUtils.h"

#include <iostream>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc < 3)
    {
        std::cerr << "usage: StashTrackDownloadProbe <url> <downloadFolder>" << std::endl;
        return 2;
    }

    const juce::String url (argv[1]);
    const juce::File folder { juce::String (argv[2]) };

    int progressEvents = 0;
    float lastPercent = -1.0f;

    const auto result = StashTrack::downloadAudioWithYtDlp (
        url, folder, StashTrack::DownloadOptions{},
        [&progressEvents, &lastPercent] (float percent)
        {
            ++progressEvents;

            if (percent != lastPercent)
            {
                std::cout << "PROGRESS " << juce::String (percent, 1) << "%" << std::endl;
                lastPercent = percent;
            }
        });

    std::cout << "--- output head ---\n"
              << result.processOutput.substring (0, 1200)
              << "\n--- output tail ---\n"
              << result.processOutput.getLastCharacters (600)
              << "\n-------------------\n";

    std::cout << "events: " << progressEvents
              << "\nsucceeded: " << (result.succeeded ? "yes" : "no")
              << "\nfile: " << result.downloadedFile.getFullPathName()
              << "\nmessage: " << result.message << std::endl;

    return result.succeeded && progressEvents > 0 ? 0 : 1;
}
