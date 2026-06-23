import type { Metadata, Viewport } from 'next'
import { Geist, Geist_Mono } from 'next/font/google'
import './globals.css'

const geistSans = Geist({
  variable: '--font-geist-sans',
  subsets: ['latin'],
  display: 'swap',
})

const geistMono = Geist_Mono({
  variable: '--font-geist-mono',
  subsets: ['latin'],
  display: 'swap',
})

const siteUrl = 'https://stashtrack.n9records.com'

export const metadata: Metadata = {
  metadataBase: new URL(siteUrl),
  title: 'StashTrack - Sample from the web inside FL Studio',
  description:
    'StashTrack is a JUCE VST3 by N9 Records. Paste a URL, clip the range, download with yt-dlp and ffmpeg, then drag the waveform into FL Studio.',
  alternates: {
    canonical: '/',
  },
  openGraph: {
    title: 'StashTrack',
    description:
      'A VST3 sample grabber for FL Studio beatmakers who want fast URL-to-playlist workflow.',
    url: siteUrl,
    siteName: 'StashTrack',
    type: 'website',
    images: [
      {
        url: '/og.svg',
        width: 1200,
        height: 630,
        alt: 'StashTrack plugin waveform preview',
      },
    ],
  },
  icons: {
    icon: '/icon.svg',
    apple: '/icon.svg',
  },
}

export const viewport: Viewport = {
  colorScheme: 'dark',
  themeColor: '#0a0b0a',
}

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode
}>) {
  return (
    <html lang="en" className={`${geistSans.variable} ${geistMono.variable}`}>
      <body>{children}</body>
    </html>
  )
}
