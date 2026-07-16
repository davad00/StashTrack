'use client'

// Motion core for the StashTrack landing: Lenis smooth scroll + GSAP
// ScrollTrigger boot, magnetic buttons, and scroll reveals.

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
      gsap.to(el, {
        x: (e.clientX - rect.left - rect.width / 2) * 0.28,
        y: (e.clientY - rect.top - rect.height / 2) * 0.28,
        duration: 0.4,
        ease: 'power2.out',
      })
    }

    const leave = () => gsap.to(el, { x: 0, y: 0, duration: 0.6, ease: 'elastic.out(1, 0.4)' })

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
