# StashTrack JUCE Plug-in

StashTrack is a JUCE audio plug-in that lets you paste a YouTube or other yt-dlp-supported URL, download the best available audio stream, show the downloaded file as a waveform, and drag that waveform directly into a DAW playlist/track as a native audio file drag.

## Metadata

Publisher: N9 Records
Website: https://stashtrack.n9records.com
Support: vsts@n9records.com
Version: v0.3
Copyright: Copyright (c) 2026 N9 Records
License: StashTrack Non-Commercial License v0.1. Free to use, copy, modify, and share for non-commercial purposes only. No commercial use or profit is allowed.

Internal VST3 compatibility note: the plug-in keeps its original JUCE manufacturer code so DAWs such as FL Studio can continue opening existing StashTrack plugin database entries after branding metadata changes.

## Legal Notice

Downloading YouTube or other website content may violate that site's Terms of Service or copyright laws. Only download content you own, have licensed, or otherwise have the rights to use.

## Runtime Dependencies

The plug-in shells out to the same class of tools used behind the scenes by Stacher7, but yt-dlp is now launched through `uvx` instead of a direct Python/yt-dlp executable. This avoids host-side Python environment issues like `Fatal Python error: init_fs_encoding`.

- `uv` / `uvx`
- `ffmpeg`
- `deno` for modern YouTube JavaScript challenge solving

The installer copies `uv.exe` and `uvx.exe` beside the built plug-in binary when it can find them. If uv is missing, the installer downloads uv with Astral's official standalone installer and installs it into the VST3 bundle. On Windows, that bundle location is:

```text
build-vs/StashTrack_artefacts/Release/VST3/StashTrack.vst3/Contents/x86_64-win/
```

`ffmpeg` must still be installed on `PATH`, or copied beside the plug-in binary when using the development install scripts. On Windows, the full `.exe` installer described below bundles `ffmpeg.exe`, `deno.exe`, `uv.exe`, `uvx.exe`, and the Microsoft Visual C++ runtime so a fresh PC does not need Python, uv, deno, or ffmpeg preinstalled.

The downloader uses a command shaped like this:

```bash
uvx --refresh-package yt-dlp --from yt-dlp@latest yt-dlp -f bestaudio -x --audio-format wav --print after_move:filepath --output "<downloadFolder>/%(title)s.%(ext)s" <URL>
```

The plug-in requests `bestaudio`, then converts to WAV so JUCE's built-in `AudioFormatManager` can reliably load the result. StashTrack forces `uvx` to refresh yt-dlp metadata and run `yt-dlp@latest` so a stale cached yt-dlp build does not break newer YouTube options.

When `Clip` is enabled, StashTrack prefers segmented m3u8 media before falling back to `bestaudio`:

```bash
uvx --refresh-package yt-dlp --from yt-dlp@latest yt-dlp --js-runtimes "deno:<pluginToolsFolder>/deno.exe" --remote-components ejs:npm -f "best[protocol*=m3u8][height<=360]/best[protocol*=m3u8]/bestaudio" --download-sections "*<start>-<end>"
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

## React-JUCE UI Bundle

The project vendors React-JUCE under `external/react-juce` and compiles the `react_juce` module into the StashTrack target when `STASHTRACK_ENABLE_REACT_JUCE=ON` (the default). React-JUCE is a JUCE/React bridge, but the waveform and external file drag are intentionally still native JUCE code because the host drag operation must use JUCE's native file-drag API.

The React-JUCE bundle source lives in `jsui/` and uses Bun, not npm:

```powershell
cd jsui
bun install
bun run build
```

Or build it through CMake:

```powershell
cmake --build build-vs --target StashTrackReactJuceUiBundle --config Release
```

If you clone this project somewhere else and `external/react-juce` is missing, restore it with:

```powershell
git clone --depth 1 https://github.com/JoshMarler/react-juce.git external/react-juce
git -C external/react-juce submodule update --init --depth 1 react_juce/yoga
```

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

The landing page download buttons point to the v0.3 GitHub Release installer:

```text
https://github.com/davad00/StashTrack/releases/download/v0.3/StashTrackv0.3Setup.exe
```

After rebuilding `dist/StashTrackv0.3Setup.exe`, upload the release asset to GitHub Releases and update `stashtrack-landing/app/page.tsx` if the release URL changes.

## License

StashTrack's original code and project materials are under the [StashTrack Non-Commercial License v0.1](LICENSE.md). You can use, copy, modify, and share it for non-commercial purposes, but nobody may profit from StashTrack or its original code without prior written permission from N9 Records.

Third-party dependencies keep their own licenses.

## Build

This repo expects JUCE to be available at `H:/code/VSTs/JUCE` by default. Override `JUCE_SOURCE_DIR` if JUCE lives somewhere else.

### Windows

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR=H:/code/VSTs/JUCE
cmake --build build-vs --target StashTrackReactJuceUiBundle --config Release
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
dist/StashTrackv0.3Setup.exe
```

`StashTrackv0.3Setup.exe` installs to the standard system VST3 folder:

```text
C:\Program Files\Common Files\VST3\StashTrack.vst3
```

The installer bundles `uv.exe`, `uvx.exe`, `ffmpeg.exe`, `deno.exe`, and the Microsoft Visual C++ Redistributable x64. The setup checks for the VC++ runtime before running the redistributable. It also removes stale `StashTrack.vst3` copies from system and user-local VST3 folders before installing, so FL Studio is less likely to keep loading an old scanned plug-in. If Inno Setup is not installed on the build machine, the builder downloads Inno Setup 6 locally and uses `ISCC.exe` to compile the installer. The target PC still needs internet access when StashTrack first downloads from a URL because `uvx` fetches/caches yt-dlp and its EJS helper package on demand.

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

- `Source/DownloadUtils.*`: URL validation, uvx/yt-dlp command construction, output parsing, and `ChildProcess` execution.
- `Source/PluginEditor.*`: JUCE GUI with `TextEditor`, `TextButton`, status `Label`, background download thread, message-thread status updates, waveform rendering, and native file drag into the host.
- `Source/PluginProcessor.*`: silent default processor path with retained JUCE audio-loading helpers for future playback workflows.
- `Tests/DownloadUtilsTests.cpp`: command/URL/output parsing tests.
- `jsui/`: Bun-built React-JUCE UI bundle source.
- `stashtrack-landing/`: Bun/Next static landing page for Render.
- `external/react-juce/`: vendored React-JUCE C++ module.
