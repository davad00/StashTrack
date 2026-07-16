'use client'

import { useEffect, useRef, useState } from 'react'
import { gsap } from 'gsap'
import { ScrollTrigger } from 'gsap/ScrollTrigger'
import styles from './page.module.css'
import {
  Cursor,
  Magnetic,
  ScrollReadout,
  WaveRiver,
  useMotionReady,
  useReveal,
} from './components/experience'

const SUPPORT_EMAIL = 'vsts@n9records.com'
const DOWNLOAD_URL = '/download/windows'
const DOWNLOAD_MACOS_URL = '/download/macos'
const DOWNLOAD_LINUX_URL = '/download/linux'

type LatestReleaseResponse = {
  versionTag?: string
}

const sessionSteps: Array<[string, string, string]> = [
  [
    '01 / PASTE',
    'Drop the link. Stay in the mixer.',
    'Any YouTube or yt-dlp-supported URL goes straight into the plugin. The browser never gets your focus back.',
  ],
  [
    '02 / CLIP',
    'Take fifteen seconds, not fifteen minutes.',
    'Mark start and end before the download begins. StashTrack prefers segment streams, so a 2-hour set gives up its best bar in seconds.',
  ],
  [
    '03 / RENDER',
    'Watch it arrive.',
    'yt-dlp pulls, ffmpeg renders a clean WAV, and the progress bar under the URL is real — parsed live from the stream.',
  ],
]

export default function Home() {
  const ready = useMotionReady()
  useReveal(ready)

  const [versionTag, setVersionTag] = useState('latest')
  const versionLabel = versionTag || 'latest'

  const heroRef = useRef<HTMLElement>(null)
  const dragSceneRef = useRef<HTMLElement>(null)
  const chipRef = useRef<HTMLDivElement>(null)
  const trackRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    let cancelled = false

    fetch('/api/latest-release')
      .then((response) => (response.ok ? response.json() : undefined))
      .then((release: LatestReleaseResponse | undefined) => {
        if (!cancelled && release?.versionTag) {
          setVersionTag(release.versionTag)
        }
      })
      .catch(() => undefined)

    return () => {
      cancelled = true
    }
  }, [])

  // Hero entrance: title lines rise, plugin window drifts in.
  useEffect(() => {
    if (!ready || !heroRef.current) return

    const lines = heroRef.current.querySelectorAll(`.${styles.titleLine}`)
    const window_ = heroRef.current.querySelector(`.${styles.pluginWindow}`)

    const tl = gsap.timeline()
    tl.from(lines, {
      y: 140,
      opacity: 0,
      duration: 1.3,
      stagger: 0.09,
      ease: 'power4.out',
    })

    if (window_) {
      tl.from(
        window_,
        { y: 80, opacity: 0, rotate: 1.5, duration: 1.4, ease: 'power4.out' },
        '-=0.9',
      )
    }

    return () => {
      tl.kill()
    }
  }, [ready])

  // Signature scene: the rendered clip drags itself from the plugin into the
  // playlist as you scroll through the pinned scene.
  useEffect(() => {
    if (!ready || !dragSceneRef.current || !chipRef.current || !trackRef.current) return
    if (window.matchMedia('(max-width: 767px)').matches) return

    const chip = chipRef.current
    const track = trackRef.current

    const tl = gsap.timeline({
      scrollTrigger: {
        trigger: dragSceneRef.current,
        start: 'top top',
        end: '+=140%',
        pin: true,
        scrub: 1,
      },
    })

    tl.to(chip, { y: 250, x: 40, rotate: 3, ease: 'none' }, 0)
      .to(chip, { boxShadow: '0 30px 80px rgba(198, 241, 53, 0.28)', ease: 'none' }, 0)
      .fromTo(
        track,
        { outlineColor: 'rgba(198, 241, 53, 0)' },
        { outlineColor: 'rgba(198, 241, 53, 0.8)', ease: 'none' },
        0.4,
      )
      .to(chip, { scale: 0.96, ease: 'none' }, 0.8)

    return () => {
      tl.scrollTrigger?.kill()
      tl.kill()
    }
  }, [ready])

  return (
    <main className={styles.page} id="top">
      <a className={styles.skipLink} href="#session">
        Skip to content
      </a>

      <Cursor />
      <WaveRiver />

      <header className={styles.nav} aria-label="Primary">
        <a href="#top" className={styles.brand} aria-label="StashTrack home">
          {/* eslint-disable-next-line @next/next/no-img-element */}
          <img src="/logos/logo-no-text.jpeg" alt="" width={30} height={30} />
          <span>StashTrack</span>
          <span className={styles.versionPill}>
            <i aria-hidden="true" />
            {versionLabel}
          </span>
        </a>
        <nav className={styles.navLinks}>
          <a href="#session">Session</a>
          <a href="#platforms">Downloads</a>
          <a href={`mailto:${SUPPORT_EMAIL}`}>Support</a>
        </nav>
        <span className={styles.readout} aria-hidden="true">
          <ScrollReadout />
        </span>
      </header>

      <section ref={heroRef} className={styles.hero}>
        <h1 className={styles.title}>
          <span className={styles.titleLine}>SAMPLE</span>
          <span className={styles.titleLine}>
            THE <em>WEB</em>
          </span>
          <span className={styles.titleLine}>IN YOUR DAW.</span>
        </h1>

        <div className={styles.heroSide}>
          <p className={styles.lede}>
            StashTrack is a VST3 that downloads the section you want, renders
            the waveform inside the plugin, and hands it to your playlist as a
            native file drag.
          </p>
          <div className={styles.actions}>
            <Magnetic>
              <a className={styles.primaryButton} href={DOWNLOAD_URL} data-hover>
                WINDOWS · {versionLabel}
              </a>
            </Magnetic>
            <a className={styles.ghostButton} href={DOWNLOAD_MACOS_URL} data-hover>
              MACOS
            </a>
            <a className={styles.ghostButton} href={DOWNLOAD_LINUX_URL} data-hover>
              LINUX
            </a>
          </div>
          <p className={styles.legal}>
            Only download content you own, have licensed, or have rights to use.
          </p>
        </div>

        <div className={styles.pluginWindow} aria-label="StashTrack plugin preview">
          <div className={styles.pluginBar}>
            <span />
            <span />
            <span className={styles.pluginTitle}>StashTrack.vst3</span>
          </div>
          <div className={styles.pluginBody}>
            <span className={styles.microLabel}>SOURCE URL</span>
            <div className={styles.urlRow}>
              <span className={styles.urlText}>youtube.com/watch?v=hsGOT_0L16U</span>
              <span className={styles.urlButton}>DOWNLOAD</span>
            </div>
            <div className={styles.pluginWave} aria-hidden="true">
              {Array.from({ length: 36 }, (_, i) => (
                <i
                  key={i}
                  style={{
                    height: `${18 + Math.round((Math.sin(i * 0.55) * 0.5 + 0.5) * 64)}%`,
                    animationDelay: `${(i % 9) * 90}ms`,
                  }}
                />
              ))}
            </div>
            <div className={styles.pluginFooter}>
              <span className={styles.microLabel}>READY</span>
              <span className={styles.microAccent}>DRAG TO PLAYLIST</span>
            </div>
          </div>
        </div>
      </section>

      <section id="session" className={styles.session}>
        {sessionSteps.map(([step, headline, body]) => (
          <article className={styles.scene} key={step} data-reveal>
            <span className={styles.microLabel}>{step}</span>
            <h2 className={styles.sceneTitle}>{headline}</h2>
            <p className={styles.sceneBody}>{body}</p>
          </article>
        ))}
      </section>

      <section ref={dragSceneRef} className={styles.dragScene} aria-label="Drag demo">
        <div className={styles.dragInner}>
          <span className={styles.microLabel}>04 / DRAG</span>
          <h2 className={styles.sceneTitle}>The waveform is the file.</h2>
          <p className={styles.sceneBody}>
            Scroll — the clip leaves the plugin and lands on the playlist,
            exactly like it does in your session. Same OS drag Explorer would
            send. No export dialog, ever.
          </p>

          <div className={styles.dragStage}>
            <div ref={chipRef} className={styles.waveChip}>
              <span className={styles.microLabel}>clip.wav · 0:15</span>
              <div className={styles.chipWave} aria-hidden="true">
                {Array.from({ length: 22 }, (_, i) => (
                  <i
                    key={i}
                    style={{ height: `${24 + Math.round((Math.sin(i * 0.7) * 0.5 + 0.5) * 56)}%` }}
                  />
                ))}
              </div>
            </div>
            <div ref={trackRef} className={styles.playlistTrack}>
              <span className={styles.microLabel}>PLAYLIST · TRACK 7</span>
            </div>
          </div>
        </div>
      </section>

      <section id="platforms" className={styles.platforms}>
        <h2 className={styles.sectionStatement} data-reveal>
          One plugin.
          <br />
          Every studio.
        </h2>

        <div className={styles.platformGrid}>
          <article className={styles.platformCard} data-reveal>
            <span className={styles.microLabel}>WINDOWS · VST3</span>
            <h3>Full installer, batteries included.</h3>
            <p>
              Bundles yt-dlp, ffmpeg, Deno, uv, and the VC++ runtime. A fresh
              PC needs nothing else — run the setup, rescan, sample.
            </p>
            <Magnetic>
              <a className={styles.primaryButton} href={DOWNLOAD_URL} data-hover>
                DOWNLOAD {versionLabel}
              </a>
            </Magnetic>
          </article>

          <article className={styles.platformCard} data-reveal>
            <span className={styles.microLabel}>MACOS · VST3 PKG</span>
            <h3>Unsigned, on purpose (for now).</h3>
            <p>
              Installs to /Library/Audio/Plug-Ins/VST3. No Apple Developer ID
              yet, so on first install: right-click the .pkg → Open, or allow
              it in Privacy &amp; Security. Tools via{' '}
              <code>brew install yt-dlp ffmpeg</code>.
            </p>
            <a className={styles.ghostButton} href={DOWNLOAD_MACOS_URL} data-hover>
              DOWNLOAD .PKG
            </a>
          </article>

          <article className={styles.platformCard} data-reveal>
            <span className={styles.microLabel}>LINUX · VST3 TAR.GZ</span>
            <h3>Untar, install.sh, done.</h3>
            <p>
              Ships the VST3 with an installer targeting ~/.vst3. Bring{' '}
              <code>ffmpeg</code> and <code>yt-dlp</code> from your package
              manager and they are found on PATH.
            </p>
            <a className={styles.ghostButton} href={DOWNLOAD_LINUX_URL} data-hover>
              DOWNLOAD .TAR.GZ
            </a>
          </article>
        </div>

        <p className={styles.updateNote} data-reveal>
          <span className={styles.microAccent}>AUTO-UPDATES</span> — StashTrack
          checks GitHub when the plugin opens. Windows installs the new version
          in place; macOS and Linux are pointed at the right download for their
          platform.
        </p>
      </section>

      <section className={styles.finalCta}>
        <h2 className={styles.ctaTitle} data-reveal>
          KEEP THE BROWSER
          <br />
          OUT OF THE BEAT.
        </h2>
        <div className={styles.actions} data-reveal>
          <Magnetic>
            <a className={styles.primaryButton} href={DOWNLOAD_URL} data-hover>
              WINDOWS · {versionLabel}
            </a>
          </Magnetic>
          <a className={styles.ghostButton} href={DOWNLOAD_MACOS_URL} data-hover>
            MACOS
          </a>
          <a className={styles.ghostButton} href={DOWNLOAD_LINUX_URL} data-hover>
            LINUX
          </a>
        </div>
      </section>

      <footer className={styles.footer}>
        <div className={styles.footerBrand}>
          {/* eslint-disable-next-line @next/next/no-img-element */}
          <img src="/logos/logo-no-text.jpeg" alt="" width={24} height={24} />
          <span>StashTrack — N9 Records</span>
        </div>
        <p>
          Support: <a href={`mailto:${SUPPORT_EMAIL}`}>{SUPPORT_EMAIL}</a>
        </p>
        <p>Rendered natively by VSReacT. Only sample what you have rights to.</p>
      </footer>
    </main>
  )
}
