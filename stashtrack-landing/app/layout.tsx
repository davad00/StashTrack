import type { Metadata, Viewport } from 'next'
import { Space_Grotesk, JetBrains_Mono } from 'next/font/google'
import './globals.css'

const grotesk = Space_Grotesk({
  variable: '--font-grotesk',
  subsets: ['latin'],
  display: 'swap',
})

const mono = JetBrains_Mono({
  variable: '--font-mono',
  subsets: ['latin'],
  display: 'swap',
})

const siteUrl = 'https://stashtrack.n9records.com'

export const metadata: Metadata = {
  metadataBase: new URL(siteUrl),
  title: 'StashTrack — Sample the web from inside your DAW',
  description:
    'StashTrack is a VST3 by N9 Records for Windows, macOS, and Linux. Paste a URL, clip the range, watch it render, drag the waveform straight into your FL Studio playlist.',
  alternates: {
    canonical: '/',
  },
  openGraph: {
    title: 'StashTrack',
    description:
      'URL to playlist track in one motion. A VST3 sample grabber for beatmakers — Windows, macOS, and Linux.',
    url: siteUrl,
    siteName: 'StashTrack',
    type: 'website',
    images: [
      {
        url: '/logos/logo-text.jpeg',
        width: 1024,
        height: 1024,
        alt: 'StashTrack — a crate of samples with a glowing waveform',
      },
    ],
  },
  icons: {
    icon: '/logos/logo-no-text.jpeg',
    apple: '/logos/logo-no-text.jpeg',
  },
}

export const viewport: Viewport = {
  colorScheme: 'dark',
  themeColor: '#070907',
}

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode
}>) {
  return (
    <html lang="en" className={`${grotesk.variable} ${mono.variable}`}>
      <body>{children}</body>
    </html>
  )
}
