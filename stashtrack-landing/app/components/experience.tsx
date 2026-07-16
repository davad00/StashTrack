'use client'

// Motion core for the StashTrack breakthrough experience: Lenis smooth
// scroll, GSAP ScrollTrigger wiring, the waveform river (the signature —
// scrolling the page IS downloading the sample), the custom cursor, and
// magnetic interactive elements.

import { useEffect, useRef, useState, type ReactNode } from 'react'
import { gsap } from 'gsap'
import { ScrollTrigger } from 'gsap/ScrollTrigger'
import Lenis from 'lenis'

let motionBooted = false

export function useMotionReady(): boolean {
  const [ready, setReady] = useState(false)

  useEffect(() => {
    const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches
    const touch = window.matchMedia('(pointer: coarse)').matches

    if (reduced) return

    gsap.registerPlugin(ScrollTrigger)

    let lenis: Lenis | undefined
    let tick: ((time: number) => void) | undefined

    if (!motionBooted) {
      motionBooted = true

      if (!touch) {
        lenis = new Lenis({ lerp: 0.12 })
        lenis.on('scroll', ScrollTrigger.update)
        tick = (time: number) => lenis!.raf(time * 1000)
        gsap.ticker.add(tick)
        gsap.ticker.lagSmoothing(0)
      }
    }

    setReady(true)

    return () => {
      if (lenis) {
        if (tick) gsap.ticker.remove(tick)
        lenis.destroy()
        motionBooted = false
      }
    }
  }, [])

  return ready
}

/** The waveform river: a fixed canvas behind the page. Bars ahead of the
    scroll playhead are embers; bars behind it are lit lime — the page
    downloads as you read it. */
export function WaveRiver() {
  const canvasRef = useRef<HTMLCanvasElement>(null)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return

    const context = canvas.getContext('2d')
    if (!context) return

    const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches

    let width = 0
    let height = 0
    let raf = 0
    let progress = 0
    let phase = 0

    const resize = () => {
      const ratio = Math.min(window.devicePixelRatio || 1, 2)
      width = window.innerWidth
      height = window.innerHeight
      canvas.width = width * ratio
      canvas.height = height * ratio
      canvas.style.width = `${width}px`
      canvas.style.height = `${height}px`
      context.setTransform(ratio, 0, 0, ratio, 0, 0)
    }

    const readProgress = () => {
      const max = document.documentElement.scrollHeight - window.innerHeight
      progress = max > 0 ? Math.min(1, Math.max(0, window.scrollY / max)) : 0
    }

    const barAmplitude = (index: number, total: number) => {
      const t = index / total
      const a = Math.sin(index * 0.35 + phase) * 0.5 + 0.5
      const b = Math.sin(index * 0.11 - phase * 0.6 + 2.1) * 0.5 + 0.5
      const envelope = 0.35 + 0.65 * Math.sin(t * Math.PI)
      return (a * 0.62 + b * 0.38) * envelope
    }

    const draw = () => {
      readProgress()
      context.clearRect(0, 0, width, height)

      const barWidth = 5
      const gap = 4
      const total = Math.ceil(width / (barWidth + gap))
      const centerY = height * 0.5
      const maxBar = height * 0.32
      const playhead = progress * total

      for (let i = 0; i < total; i++) {
        const amplitude = barAmplitude(i, total)
        const barHeight = Math.max(3, amplitude * maxBar)
        const x = i * (barWidth + gap)
        const lit = i <= playhead

        if (lit) {
          context.fillStyle = `rgba(198, 241, 53, ${0.34 + amplitude * 0.4})`
        } else {
          context.fillStyle = `rgba(121, 130, 110, ${0.07 + amplitude * 0.08})`
        }

        context.fillRect(x, centerY - barHeight, barWidth, barHeight * 2)
      }

      // Playhead beam.
      const beamX = Math.min(width - 1, progress * width)
      const beam = context.createLinearGradient(beamX - 40, 0, beamX + 1, 0)
      beam.addColorStop(0, 'rgba(198, 241, 53, 0)')
      beam.addColorStop(1, 'rgba(198, 241, 53, 0.55)')
      context.fillStyle = beam
      context.fillRect(beamX - 40, centerY - maxBar, 41, maxBar * 2)

      if (!reduced) phase += 0.012
      raf = requestAnimationFrame(draw)
    }

    resize()
    window.addEventListener('resize', resize)

    if (reduced) {
      readProgress()
      draw()
      cancelAnimationFrame(raf)
    } else {
      raf = requestAnimationFrame(draw)
    }

    return () => {
      cancelAnimationFrame(raf)
      window.removeEventListener('resize', resize)
    }
  }, [])

  return (
    <canvas
      ref={canvasRef}
      aria-hidden="true"
      style={{
        position: 'fixed',
        inset: 0,
        zIndex: 0,
        opacity: 0.9,
        pointerEvents: 'none',
      }}
    />
  )
}

/** Live download readout tied to scroll — the numeric voice of the river. */
export function ScrollReadout() {
  const ref = useRef<HTMLSpanElement>(null)

  useEffect(() => {
    const el = ref.current
    if (!el) return

    let raf = 0

    const update = () => {
      const max = document.documentElement.scrollHeight - window.innerHeight
      const value = max > 0 ? Math.round((window.scrollY / max) * 100) : 0
      el.textContent = `RECEIVING ${String(value).padStart(3, '0')}%`
      raf = requestAnimationFrame(update)
    }

    raf = requestAnimationFrame(update)
    return () => cancelAnimationFrame(raf)
  }, [])

  return <span ref={ref}>RECEIVING 000%</span>
}

export function Cursor() {
  const dotRef = useRef<HTMLDivElement>(null)
  const ringRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const fine = window.matchMedia('(pointer: fine)').matches
    const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches
    if (!fine || reduced) return

    document.body.dataset.customCursor = 'true'

    const dot = dotRef.current!
    const ring = ringRef.current!
    dot.style.display = 'block'
    ring.style.display = 'block'

    const dotX = gsap.quickTo(dot, 'x', { duration: 0.12, ease: 'power3.out' })
    const dotY = gsap.quickTo(dot, 'y', { duration: 0.12, ease: 'power3.out' })
    const ringX = gsap.quickTo(ring, 'x', { duration: 0.5, ease: 'power3.out' })
    const ringY = gsap.quickTo(ring, 'y', { duration: 0.5, ease: 'power3.out' })

    const move = (e: MouseEvent) => {
      dotX(e.clientX)
      dotY(e.clientY)
      ringX(e.clientX)
      ringY(e.clientY)
    }

    const over = (e: MouseEvent) => {
      const interactive = (e.target as HTMLElement).closest('a, button, [data-hover]')
      gsap.to(ring, {
        scale: interactive ? 1.9 : 1,
        opacity: interactive ? 1 : 0.6,
        duration: 0.3,
        ease: 'back.out(1.7)',
      })
    }

    window.addEventListener('mousemove', move)
    window.addEventListener('mouseover', over)

    return () => {
      window.removeEventListener('mousemove', move)
      window.removeEventListener('mouseover', over)
      delete document.body.dataset.customCursor
    }
  }, [])

  const base = {
    position: 'fixed' as const,
    top: 0,
    left: 0,
    zIndex: 100,
    pointerEvents: 'none' as const,
    borderRadius: '50%',
    display: 'none',
    mixBlendMode: 'difference' as const,
  }

  return (
    <>
      <div
        ref={dotRef}
        aria-hidden="true"
        style={{
          ...base,
          width: 10,
          height: 10,
          marginLeft: -5,
          marginTop: -5,
          background: '#c6f135',
        }}
      />
      <div
        ref={ringRef}
        aria-hidden="true"
        style={{
          ...base,
          width: 38,
          height: 38,
          marginLeft: -19,
          marginTop: -19,
          border: '1.5px solid #c6f135',
          opacity: 0.6,
        }}
      />
    </>
  )
}

export function Magnetic({ children, className }: { children: ReactNode; className?: string }) {
  const ref = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const el = ref.current
    if (!el) return

    const fine = window.matchMedia('(pointer: fine)').matches
    const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches
    if (!fine || reduced) return

    const move = (e: MouseEvent) => {
      const rect = el.getBoundingClientRect()
      const relX = e.clientX - rect.left - rect.width / 2
      const relY = e.clientY - rect.top - rect.height / 2
      gsap.to(el, { x: relX * 0.28, y: relY * 0.28, duration: 0.4, ease: 'power2.out' })
    }

    const leave = () => {
      gsap.to(el, { x: 0, y: 0, duration: 0.6, ease: 'elastic.out(1, 0.4)' })
    }

    el.addEventListener('mousemove', move)
    el.addEventListener('mouseleave', leave)

    return () => {
      el.removeEventListener('mousemove', move)
      el.removeEventListener('mouseleave', leave)
    }
  }, [])

  return (
    <div ref={ref} className={className} style={{ display: 'inline-block' }}>
      {children}
    </div>
  )
}

/** Rises a block from y+opacity when it enters the viewport. */
export function useReveal(ready: boolean) {
  useEffect(() => {
    if (!ready) return

    const targets = gsap.utils.toArray<HTMLElement>('[data-reveal]')

    const tweens = targets.map((el) =>
      gsap.from(el, {
        scrollTrigger: {
          trigger: el,
          start: 'top 85%',
          toggleActions: 'play none none reverse',
        },
        y: 90,
        opacity: 0,
        duration: 1.1,
        ease: 'power4.out',
      }),
    )

    return () => {
      tweens.forEach((t) => {
        t.scrollTrigger?.kill()
        t.kill()
      })
    }
  }, [ready])
}
