# StashTrack Landing Page

Live marketing and download site for `stashtrack.n9records.com`.

StashTrack is a JUCE VST3 by N9 Records for FL Studio beatmakers. Paste a URL,
clip the range, download with uvx/yt-dlp/Deno/ffmpeg, preview the waveform, and
drag the WAV straight into the playlist.

## Stack

- Next.js 16 App Router
- React 19
- CSS Modules
- Bun for install/build/start
- Render free Node web service

## Local Development

```bash
bun install
bun run dev
```

## Production Check

```bash
bun install --frozen-lockfile
bun run typecheck
bun run build
bun run start -- -H 0.0.0.0 -p 3000
```

## Installer Download

The download buttons point to the same-origin installer:

```text
/downloads/StashTrackv0.1Setup.exe
```

Keep this file in sync with the latest root build output:

```text
public/downloads/StashTrackv0.1Setup.exe
```

The v0.1 GitHub Release asset mirrors the same installer:

```text
https://github.com/davad00/StashTrack/releases/download/v0.1/StashTrackv0.1Setup.exe
```

## Render

The repository root includes `render.yaml`. It configures a Render web service
with:

- `runtime: node`
- `plan: free`
- `rootDir: stashtrack-landing`
- `buildCommand: bun install --frozen-lockfile && bun run build`
- `startCommand: bun run start -- -H 0.0.0.0 -p $PORT`
- custom domain: `stashtrack.n9records.com`

## Legal

Only download content you own, have licensed, or have rights to use.

## Support

Questions: [vsts@n9records.com](mailto:vsts@n9records.com)
