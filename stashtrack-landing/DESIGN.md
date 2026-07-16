---
version: alpha
name: StashTrack Breakthrough
description: Cinematic dark-room identity for the StashTrack landing experience
colors:
  primary: "#070907"
  secondary: "#10140E"
  tertiary: "#C6F135"
  neutral: "#EDF1E4"
  muted: "#79826E"
typography:
  display-huge:
    fontFamily: Space Grotesk
    fontSize: 11vw
    fontWeight: 700
    lineHeight: 0.88
    letterSpacing: -0.04em
  display-section:
    fontFamily: Space Grotesk
    fontSize: 4.5vw
    fontWeight: 700
    lineHeight: 0.95
    letterSpacing: -0.03em
  body:
    fontFamily: Space Grotesk
    fontSize: 17px
    fontWeight: 400
    lineHeight: 1.55
    letterSpacing: 0em
  label-micro:
    fontFamily: JetBrains Mono
    fontSize: 11px
    fontWeight: 500
    lineHeight: 1
    letterSpacing: 0.14em
rounded:
  none: 0px
  soft: 10px
  full: 999px
spacing:
  section: 22vh
  gutter: 5vw
components:
  button-primary:
    backgroundColor: "{colors.tertiary}"
    textColor: "{colors.primary}"
    rounded: "{rounded.full}"
    padding: 18px
    typography: "{typography.label-micro}"
  button-ghost:
    backgroundColor: "{colors.primary}"
    textColor: "{colors.neutral}"
    rounded: "{rounded.full}"
    padding: 18px
    typography: "{typography.label-micro}"
  card:
    backgroundColor: "{colors.secondary}"
    textColor: "{colors.neutral}"
    rounded: "{rounded.soft}"
    padding: 28px
---

## Overview

StashTrack lives where beatmakers live: a dark studio at 2AM, one lime-green
waveform glowing on a black console. The site is that room. Near-black canvas,
one radioactive accent, type set like hardware silkscreen. The scroll is a
session: paste, clip, render, drag — the story of one sample travelling from
the web into a playlist track.

Motion tokens: `ease-cinematic: cubic-bezier(0.16, 1, 0.3, 1)` for reveals,
scrub-bound timelines for the narrative scenes. Durations: micro 0.25s,
standard 0.7s, cinematic 1.4s+.

## Colors

`primary #070907` is the room. `secondary #10140E` is the console face —
panels barely lighter than the void. `tertiary #C6F135` is the ONLY light
source: waveform, cursor ring, buttons, live version pill. `neutral #EDF1E4`
for headlines and body, `muted #79826E` for silkscreen labels. Never more
than one lime element competing per viewport. Contrast: lime-on-primary
12.4:1, neutral-on-primary 17.1:1, muted-on-primary 5.0:1 — all AA+.

## Typography

Space Grotesk carries the architecture: the hero wordmark at 11vw, section
statements at 4.5vw. JetBrains Mono carries the machine voice: 11px uppercase
tracked labels for everything operational (versions, platform names, steps).
No middle sizes — a line is either a monument or a serial number. Kinetic
behavior: headlines rise from `y:120, opacity:0` with `power4.out`, split by
line; micro-labels appear instantly (they're instrumentation, not drama).

## Layout

No grid. A single fluid column with elements pushed off-axis: hero type bleeds
toward the left edge, the plugin window floats right, scene captions sit at
5vw gutters. Sections breathe with 22vh spacing. Lenis smooth scroll; a
full-height waveform canvas runs behind everything — the page floats over it.

## Elevation & Depth

Z-layers: `wave-canvas (0)` — the full-height waveform river; `content (10)`;
`plugin-window (12)` with a soft 60px lime-tinted shadow; `nav (20)`;
`cursor (100)`. Parallax: captions drift at 0.9, the canvas river moves at
0.35 of scroll speed — the sample feels submerged under the page.

## Shapes

Rectangles with 10px radii for consoles (matching the plugin's own cards);
pills for anything clickable; raw right angles for the canvas river. The only
organic shape on the page is the waveform itself.

## Components

- `button-primary`: lime pill, black mono label, magnetic (24px radius),
  scales 1.04 on hover with `back.out(1.7)`.
- `button-ghost`: 1px `#2A3324` border pill, neutral label; border ignites to
  lime on hover.
- `nav`: fixed top strip, logo left, mono links right, backdrop-blurred —
  links are silkscreen, discoverable instantly.
- `cursor`: 12px lime dot + 36px trailing ring, `mix-blend-mode: difference`;
  ring swells to 64px over interactive elements; hidden on touch devices.
- `version-pill`: mono, live from /api/latest-release, pulsing lime dot.

## Do's and Don'ts

- DO drive the waveform river from scroll — it is the narrative spine.
- DO respect prefers-reduced-motion: static frames, no scrub scenes.
- DON'T introduce a second accent color, ever.
- DON'T hardcode versioned installer URLs (release resolution is dynamic).
- DON'T use the browser cursor on desktop; DON'T hijack touch devices.
- DON'T let any text fall below AA contrast on the near-black base.
