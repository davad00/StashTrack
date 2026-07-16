# StashTrack JUCE Plug-in

StashTrack is a JUCE audio plug-in that lets you paste a YouTube or other yt-dlp-supported URL, download the best available audio stream, show the downloaded file as a waveform, and drag that waveform directly into a DAW playlist/track as a native audio file drag.

## Metadata

Publisher: N9 Records
Website: https://stashtrack.n9records.com
Support: vsts@n9records.com
Version: v0.6
Copyright: Copyright (c) 2026 N9 Records
License: StashTrack Non-Commercial License v0.1. Free to use, copy, modify, and share for non-commercial purposes only. No commercial use or profit is allowed.

Internal VST3 compatibility note: the plug-in keeps its original JUCE manufacturer code so DAWs such as FL Studio can continue opening existing StashTrack plugin database entries after branding metadata changes.

## Legal Notice

Downloading YouTube or other website content may violate that site's Terms of Service or copyright laws. Only download content you own, have licensed, or otherwise have the rights to use.

## Runtime Dependencies

The plug-in shells out to the same class of tools used behind the scenes by Stacher7. The Windows `.exe` installer now bundles and launches `yt-dlp.exe` directly so a fresh PC does not depend on Python or on whatever `yt-dlp` version `uvx` happens to resolve.

- `yt-dlp`
- `uv` / `uvx`
- `ffmpeg`
- `deno` for modern YouTube JavaScript challenge solving

The installer copies `yt-dlp.exe`, `uv.exe`, and `uvx.exe` beside the built plug-in binary. If uv is missing, the installer downloads uv with Astral's official standalone installer and installs it into the VST3 bundle as a fallback. On Windows, that bundle location is:

```text
build-vs/StashTrack_artefacts/Release/VST3/StashTrack.vst3/Contents/x86_64-win/
```

`ffmpeg` must still be installed on `PATH`, or copied beside the plug-in binary when using the development install scripts. On Windows, the full `.exe` installer described below bundles `yt-dlp.exe`, `ffmpeg.exe`, `deno.exe`, `uv.exe`, `uvx.exe`, and the Microsoft Visual C++ runtime so a fresh PC does not need Python, uv, deno, yt-dlp, or ffmpeg preinstalled.

The downloader uses a command shaped like this:

```bash
yt-dlp -f bestaudio -x --audio-format wav --print after_move:filepath --output "<downloadFolder>/%(title)s.%(ext)s" <URL>
```

The plug-in requests `bestaudio`, then converts to WAV so JUCE's built-in `AudioFormatManager` can reliably load the result. StashTrack prefers bundled `yt-dlp.exe`; `uvx --from yt-dlp@latest` remains only as a fallback if the bundled executable is missing.

When `Clip` is enabled, StashTrack prefers segmented m3u8 media before falling back to `bestaudio`:

```bash
yt-dlp --js-runtimes "deno:<pluginToolsFolder>/deno.exe" --remote-components ejs:npm -f "best[protocol*=m3u8][height<=360]/best[protocol*=m3u8]/bestaudio" --download-sections "*<start>-<end>"
```

This requires ffmpeg. Deno lets yt-dlp solve YouTube's current JavaScript challenges and expose m3u8 segment streams when YouTube provides them. Segment streams are much better for grabbing a short clip from a long video. If a site only exposes a single giant DASH/HTTP audio file, yt-dlp/ffmpeg may still need to scan or download more data because that is a site/format limitation.

## Download Location

On Windows, StashTrack first checks FL Studio's recent-project registry entries at:

```text
HKCU\Software\Image-Line\FL Studio <version>\MRU
```

StashTrack now only uses an MRU `.flp` when the active FL Studio window title appears to match that project name. This avoids the bad unsaved-project case where FL Studio's MRU still points at the last saved project while the current project is `Untitled`.

If no active saved `.flp` can be confidently matched, StashTrack uses the operating system's real Downloads known folder on Windows, which is the same shell location used by browsers unless the browser has its own custom override. On macOS/Linux it falls back to `~/Downloads`, then JUCE's documents folder if needed.

## Importing Into FL Studio

After a download finishes, the plug-in displays the audio waveform. Drag the waveform directly onto an empty FL Studio playlist track. StashTrack starts a native OS file drag using the downloaded WAV path, so FL Studio receives the same kind of file drop it would receive from Explorer.

StashTrack no longer plays the downloaded file through the plug-in's mixer output by default. The audio is meant to become an FL Studio audio clip after you drop it into the playlist.

## Clip Downloads

Enable `Clip`, then enter `Start` and `End` as seconds, `mm:ss`, or `hh:mm:ss`. Examples:

```text
30
1:23
2:14:05.5
```

The end time must be after the start time. StashTrack validates this before launching yt-dlp.
Clipped files include the crop range in the saved filename, for example
`Song Title [clip 1-23-45 to 1-24-00].wav`, so multiple clips from the same
source video can sit in the same folder without fighting over one output name.

## VSReacT UI (React + TypeScript)

The entire plugin UI is a React + TypeScript app in `jsui-vsreact/`, rendered
natively by the [VSReacT](../vsreact) framework (no webview): the app runs in
an embedded QuickJS engine, a custom react-reconciler streams the element tree
to C++, Yoga computes flexbox layout, and `juce::Graphics` paints every pixel.
Styling uses Tailwind-style utility classes. Only the waveform + external file
drag stay native C++ (registered as the `"waveform"` NativeView) because the
host drag must use JUCE's native file-drag API.

The framework lives one directory up at `../vsreact` (see `VSREACT_DIR` in
CMake). The UI bundle builds with Bun:

```powershell
cd jsui-vsreact
bun install
bun run build
```

Or through CMake:

```powershell
cmake --build build-vs --target StashTrackUiBundle --config Release
```

Debug/dev builds watch the bundle file: rebuild it while the plugin is open
and the UI hot-reloads in place — no DAW restart.

## Landing Page

The product site lives in `stashtrack-landing/` and is configured for Render as a free Node web service. The root `render.yaml` uses Bun, builds only that folder, starts the Next server with Render's `$PORT`, and attaches the custom domain:

```text
https://stashtrack.n9records.com
```

Local landing-page checks:

```powershell
cd stashtrack-landing
bun install
bun run typecheck
bun run build
```

The landing page download buttons point to the site redirect route:

```text
/download/windows
```

That route asks GitHub for the latest release and redirects to the newest
`StashTrackv*Setup.exe` asset. If GitHub's JSON API is unavailable, it falls
back to GitHub's `/releases/latest` redirect to infer the newest tag, then uses
the predictable versioned installer URL. As a final safety fallback, each
release should also upload a second copy named `StashTrackSetup.exe`; GitHub's
versionless `releases/latest/download/StashTrackSetup.exe` URL will always
resolve to the newest release with that asset name.

After rebuilding `dist/StashTrackv0.6Setup.exe`, upload it to GitHub Releases
both as `StashTrackv0.6Setup.exe` and as `StashTrackSetup.exe`. Future installer
version changes should not need a Render deploy as long as the release tag and
asset filenames keep those patterns.

Visible version labels on the landing page call `/api/latest-release`, which
uses the same GitHub latest-release resolver.

## Auto Updates

When the plug-in editor opens, StashTrack checks the latest GitHub Release on a
background thread. If the release tag is newer than the running
`JucePlugin_VersionString`, it prompts the user to download the installer. When
accepted, StashTrack downloads the newest `StashTrackv*Setup.exe` to the user's
Downloads folder and opens it. The update prompt also includes a Changelog
button that opens the GitHub release notes for the available version.

The installer cannot safely replace a VST3 binary while the DAW has it loaded,
so the user still needs to close FL Studio before completing setup, then reopen
FL Studio and rescan if needed.

## License

StashTrack's original code and project materials are under the [StashTrack Non-Commercial License v0.1](LICENSE.md). You can use, copy, modify, and share it for non-commercial purposes, but nobody may profit from StashTrack or its original code without prior written permission from N9 Records.

Third-party dependencies keep their own licenses.

## Build

This repo expects JUCE to be available at `H:/code/VSTs/JUCE` by default. Override `JUCE_SOURCE_DIR` if JUCE lives somewhere else.

### Windows

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR=H:/code/VSTs/JUCE
cmake --build build-vs --target StashTrackUiBundle --config Release
cmake --build build-vs --target StashTrack_VST3 --config Release
ctest --test-dir build-vs -C Release --output-on-failure
```

The built VST3 will be under:

```text
build-vs/StashTrack_artefacts/Release/VST3/
```

To build a real Windows installer for a clean PC, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File installer/windows/build-installer.ps1 -Configuration Release
```

The installer is written to:

```text
dist/StashTrackv0.6Setup.exe
```

`StashTrackv0.6Setup.exe` installs to the standard system VST3 folder:

```text
C:\Program Files\Common Files\VST3\StashTrack.vst3
```

The installer bundles `yt-dlp.exe`, `uv.exe`, `uvx.exe`, `ffmpeg.exe`, `deno.exe`, and the Microsoft Visual C++ Redistributable x64. The setup checks for the VC++ runtime before running the redistributable. It also removes stale `StashTrack.vst3` copies from system and user-local VST3 folders before installing, so FL Studio is less likely to keep loading an old scanned plug-in. If Inno Setup is not installed on the build machine, the builder downloads Inno Setup 6 locally and uses `ISCC.exe` to compile the installer. The target PC still needs internet access when StashTrack first downloads from a URL because yt-dlp/Deno may fetch YouTube helper components on demand.

For a simple install after building, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File installer/install-stashtrack.ps1 -Configuration Release
```

On Windows this installs to the standard VST3 folder used by FL Studio and most DAWs:

```text
C:\Program Files\Common Files\VST3\StashTrack.vst3
```

The script will request administrator permission if needed.

To also copy a local `ffmpeg.exe` into the VST3 bundle, pass its folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File installer/install-stashtrack.ps1 -Configuration Release -ToolsDir C:/tools/ffmpeg
```

To install only for the current Windows user, use `-User`. FL Studio may not scan this folder by default:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File installer/install-stashtrack.ps1 -Configuration Release -User
```

To copy to the system VST3 folder as part of the CMake build, configure CMake with:

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR=H:/code/VSTs/JUCE -DSTASHTRACK_COPY_PLUGIN_AFTER_BUILD=ON
```

That may require an elevated shell.

### macOS

```bash
cmake -S . -B build -DJUCE_SOURCE_DIR=/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build --target StashTrack_VST3
ctest --test-dir build --output-on-failure
./installer/install-stashtrack.sh --configuration Release --build-dir ./build
```

The CMake project also builds AU on Apple platforms.

### Linux

```bash
cmake -S . -B build -DJUCE_SOURCE_DIR=/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build --target StashTrack_VST3
ctest --test-dir build --output-on-failure
./installer/install-stashtrack.sh --configuration Release --build-dir ./build
```

## Source Layout

- `Source/DownloadUtils.*`: URL validation, yt-dlp command construction, output parsing, and `ChildProcess` execution.
- `Source/UpdateUtils.*`: latest-release lookup, semantic version comparison, update installer download, and installer launch.
- `Source/PluginEditor.*`: hosts the VSReacT React UI, the background download thread, message-thread status events, and the native waveform + file-drag component.
- `Source/PluginProcessor.*`: silent default processor path with retained JUCE audio-loading helpers for future playback workflows.
- `Tests/DownloadUtilsTests.cpp`: command/URL/output parsing tests.
- `jsui-vsreact/`: the React + TypeScript UI app rendered by VSReacT.
- `stashtrack-landing/`: Bun/Next static landing page for Render.
