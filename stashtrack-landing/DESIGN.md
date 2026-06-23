---
name: stashtrack
type: website
brand:
  name: StashTrack
  tagline: "URL-to-playlist sampling for FL Studio beatmakers"
  personality: [focused, nocturnal, technical, tactile]
  voice: [direct, specific, studio-native]
colors:
  palette:
    primary: "#0A0B0A"
    secondary: "#E8EAE6"
    accent: "#C6F135"
    background: "#0A0B0A"
    surface: "#101210"
    text-primary: "#E8EAE6"
    text-secondary: "#9AA097"
    border: "#2B3029"
    error: "#FF4F64"
    success: "#C6F135"
  semantics:
    - label: "Primary action"
      token: "--accent"
    - label: "Main background"
      token: "--bg"
    - label: "Panel surface"
      token: "--panel"
    - label: "Muted text"
      token: "--muted"
typography:
  font-family:
    primary: "Geist, system-ui, sans-serif"
    secondary: "Geist, system-ui, sans-serif"
    mono: "Geist Mono, ui-monospace, monospace"
  scale:
    display: { size: "5rem", weight: 800, line-height: 1 }
    h1: { size: "3.5rem", weight: 800, line-height: 1 }
    h2: { size: "2.35rem", weight: 800, line-height: 1 }
    h3: { size: "1.2rem", weight: 700, line-height: 1.2 }
    body: { size: "1rem", weight: 400, line-height: 1.7 }
    caption: { size: "0.78rem", weight: 400, line-height: 1.5 }
spacing:
  scale:
    xs: "0.25rem"
    sm: "0.5rem"
    md: "1rem"
    lg: "1.5rem"
    xl: "2rem"
    2xl: "3rem"
    3xl: "4.25rem"
motion:
  easing:
    default: "cubic-bezier(0.2, 0, 0, 1)"
    enter: "cubic-bezier(0.16, 1, 0.3, 1)"
    exit: "cubic-bezier(0.4, 0, 1, 1)"
    bounce: "cubic-bezier(0.2, 0, 0, 1)"
  duration:
    fast: "180ms"
    normal: "260ms"
    slow: "420ms"
  principles:
    - Motion should feel like a piece of studio hardware waking up.
    - Animate opacity and transforms only.
    - Disable waveform animation for reduced motion.
border-radius:
  scale:
    none: "0"
    sm: "0.125rem"
    md: "0.25rem"
    lg: "0.5rem"
    full: "9999px"
shadows:
  scale:
    none: "none"
    sm: "0 8px 20px rgba(0, 0, 0, 0.2)"
    md: "0 24px 60px rgba(0, 0, 0, 0.36)"
    lg: "0 30px 80px rgba(0, 0, 0, 0.42)"
    xl: "0 40px 100px rgba(0, 0, 0, 0.5)"
z-index:
  scale:
    base: 0
    dropdown: 100
    sticky: 200
    modal: 300
    toast: 400
---

# Design Language

## Overview
StashTrack uses a nocturnal studio-terminal identity: off-black surfaces, thin grid lines, lime signal accents, mono labels, and tactile rectangular controls. The audience is FL Studio beatmakers who want a fast sample-grabbing tool that feels like part of the DAW, not a marketing wrapper.

## Do's and Don'ts
### Do's
- Use lime only for actions, status, waveform energy, and active clip boundaries.
- Keep panels squared, dense, and hardware-like.
- Use mono text for timecodes, file states, URLs, and workflow labels.
- Let the waveform be the visual hero.

### Don'ts
- Do not use purple, blue gradients, rounded SaaS cards, or lifestyle imagery.
- Do not center every section; keep layouts left-biased and workbench-like.
- Do not add decorative icons when text and waveform geometry carry the product.
- Do not animate layout dimensions.

## Signature Moment
The signature interaction is the waveform as a file handle: on the landing page it animates and drops into a track mock, while in the VST it becomes a native OS drag source into the FL Studio playlist. Reduced-motion users receive a static waveform.

## Accessibility
- Text contrast must meet WCAG AA.
- Controls must keep visible focus states.
- Touch targets must be at least 44px high.
- Motion respects `prefers-reduced-motion`.
