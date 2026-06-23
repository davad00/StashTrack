'use client'

import { useMemo, useState } from 'react'
import styles from './page.module.css'

const SUPPORT_EMAIL = 'vsts@n9records.com'
const DOWNLOAD_URL =
  'https://github.com/davad00/StashTrack/releases/download/v0.1/StashTrackv0.1Setup.exe'

const bars = Array.from({ length: 84 }, (_, index) => {
  const a = Math.sin(index * 0.47) * 0.5 + 0.5
  const b = Math.sin(index * 0.19 + 1.7) * 0.5 + 0.5
  return Math.max(18, Math.round((a * 0.66 + b * 0.34) * 100))
})

const steps = [
  ['Paste', 'Drop a YouTube or yt-dlp supported URL into the plugin.'],
  ['Clip', 'Mark the exact section before the download starts.'],
  ['Render', 'StashTrack pulls a WAV through uvx, yt-dlp, Deno, and ffmpeg.'],
  ['Drag', 'Grab the waveform and drop it on an empty FL Studio playlist track.'],
]

const specs = [
  ['Format', 'VST3 for Windows'],
  ['Runtime', 'Bundled uvx, Deno, ffmpeg'],
  ['Output', 'Drag-ready WAV'],
  ['Version', 'v0.1'],
]

function Waveform({ compact = false }: { compact?: boolean }) {
  return (
    <div className={compact ? styles.waveCompact : styles.wave} aria-hidden="true">
      {bars.map((height, index) => (
        <span
          key={index}
          className={styles.waveBar}
          style={{
            height: `${compact ? Math.max(12, height * 0.6) : height}%`,
            animationDelay: `${(index % 10) * 70}ms`,
          }}
        />
      ))}
    </div>
  )
}

export default function Home() {
  const [dropped, setDropped] = useState(false)
  const year = useMemo(() => new Date().getFullYear(), [])

  return (
    <main className={styles.page}>
      <a className={styles.skipLink} href="#content">
        Skip to content
      </a>

      <header className={styles.nav}>
        <a href="#top" className={styles.brand} aria-label="StashTrack home">
          <span className={styles.mark} aria-hidden="true" />
          <span>StashTrack</span>
          <span className={styles.version}>v0.1</span>
        </a>
        <nav className={styles.navLinks} aria-label="Primary navigation">
          <a href="#workflow">Workflow</a>
          <a href="#clip">Clip</a>
          <a href="#install">Install</a>
          <a href={`mailto:${SUPPORT_EMAIL}`}>Support</a>
        </nav>
        <a className={styles.navButton} href={DOWNLOAD_URL}>
          Get installer
        </a>
      </header>

      <section id="top" className={styles.hero}>
        <div className={styles.heroCopy}>
          <p className={styles.kicker}>N9 Records - for FL Studio beatmakers</p>
          <h1 className={styles.title}>Sample from the web without leaving the mixer.</h1>
          <p className={styles.lede}>
            StashTrack is a JUCE VST3 that downloads the section you want,
            renders a waveform inside the plugin, and turns it into a native
            file drag for your FL Studio playlist.
          </p>
          <div className={styles.actions}>
            <a className={styles.primaryButton} href={DOWNLOAD_URL}>
              Download StashTrack v0.1
            </a>
            <a className={styles.secondaryButton} href="#workflow">
              See the workflow
            </a>
          </div>
          <p className={styles.note}>
            Only download content you own, have licensed, or have rights to use.
          </p>
        </div>

        <aside className={styles.pluginFrame} aria-label="StashTrack plugin interface preview">
          <div className={styles.windowBar}>
            <span className={styles.windowDot} />
            <span className={styles.windowDot} />
            <span className={styles.windowDot} />
            <span className={styles.windowTitle}>StashTrack.vst3</span>
          </div>
          <div className={styles.pluginBody}>
            <div className={styles.pluginTop}>
              <span className={styles.pluginName}>StashTrack</span>
              <span className={styles.pluginStatus}>READY</span>
            </div>
            <label className={styles.mockLabel}>SOURCE URL</label>
            <div className={styles.urlMock}>
              <span>https://www.youtube.com/watch?v=hsGOT_0L16U</span>
              <button type="button">Download</button>
            </div>
            <div className={styles.clipRow}>
              <span className={styles.check}>Clip</span>
              <span>Start 1:23:45</span>
              <span>End 1:24:00</span>
            </div>
            <div className={styles.wavePanel}>
              <div className={styles.waveHeader}>
                <span>15.01 second WAV</span>
                <span>drag source armed</span>
              </div>
              <Waveform />
            </div>
          </div>
        </aside>
      </section>

      <section id="content" className={styles.stats} aria-label="Product facts">
        {specs.map(([label, value]) => (
          <div className={styles.stat} key={label}>
            <span>{label}</span>
            <strong>{value}</strong>
          </div>
        ))}
      </section>

      <section id="workflow" className={styles.workflow}>
        <div className={styles.sectionIntro}>
          <p className={styles.kicker}>Workflow</p>
          <h2>From URL to playlist track in one focused motion.</h2>
        </div>
        <ol className={styles.steps}>
          {steps.map(([title, body], index) => (
            <li key={title} className={styles.step}>
              <span className={styles.stepIndex}>{String(index + 1).padStart(2, '0')}</span>
              <h3>{title}</h3>
              <p>{body}</p>
            </li>
          ))}
        </ol>
      </section>

      <section id="clip" className={styles.split}>
        <div className={styles.sectionIntro}>
          <p className={styles.kicker}>Clip first</p>
          <h2>Long videos stop being a problem.</h2>
          <p>
            Modern YouTube extraction can expose huge DASH files. StashTrack
            prefers segment streams for clipped downloads, uses Deno for current
            YouTube JavaScript solving, and falls back cleanly when a source
            cannot provide a seek-friendly stream.
          </p>
        </div>
        <div className={styles.clipVisual}>
          <Waveform />
          <div className={styles.clipWindow}>
            <span>1:23:45</span>
            <span>1:24:00</span>
          </div>
        </div>
      </section>

      <section id="drag" className={styles.dragLab}>
        <div className={styles.sectionIntro}>
          <p className={styles.kicker}>Drag source</p>
          <h2>The waveform is the file handle.</h2>
          <p>
            FL Studio receives the same kind of file drop it would get from
            Explorer, but the selection, download, and preview all happen inside
            the plugin window.
          </p>
        </div>
        <div className={styles.dragDemo}>
          <div className={styles.dragSource}>
            <span>clip.wav</span>
            <Waveform compact />
            <button type="button" onClick={() => setDropped((value) => !value)}>
              {dropped ? 'Reset track' : 'Drop on track'}
            </button>
          </div>
          <div className={styles.track} data-filled={dropped ? 'true' : 'false'}>
            {dropped ? <Waveform compact /> : <span>EMPTY PLAYLIST TRACK</span>}
          </div>
        </div>
      </section>

      <section id="install" className={styles.install}>
        <div className={styles.sectionIntro}>
          <p className={styles.kicker}>Fresh PC install</p>
          <h2>The installer brings the toolchain with it.</h2>
          <p>
            The Windows setup checks before installing, then places the VST3 and
            helper tools beside the plugin binary. No global Python setup, no
            manual ffmpeg hunt, no guessing where uvx lives.
          </p>
        </div>
        <ul className={styles.installList}>
          <li>StashTrack.vst3</li>
          <li>uv.exe and uvx.exe</li>
          <li>deno.exe for YouTube JS solving</li>
          <li>ffmpeg.exe for decode and trim</li>
          <li>VC++ runtime when missing</li>
        </ul>
      </section>

      <section id="download" className={styles.finalCta}>
        <p className={styles.kicker}>stashtrack.n9records.com</p>
        <h2>Keep the browser out of the beat.</h2>
        <a className={styles.primaryButton} href={DOWNLOAD_URL}>
          Get StashTrack v0.1
        </a>
        <p>
          Support: <a href={`mailto:${SUPPORT_EMAIL}`}>{SUPPORT_EMAIL}</a>
        </p>
      </section>

      <footer className={styles.footer}>
        <div className={styles.brand}>
          <span className={styles.mark} aria-hidden="true" />
          <span>StashTrack</span>
        </div>
        <p>Copyright (c) {year} N9 Records.</p>
        <p>Only download content you have the rights to use.</p>
      </footer>
    </main>
  )
}
