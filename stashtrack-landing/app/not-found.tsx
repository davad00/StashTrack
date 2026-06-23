import Link from 'next/link'
import styles from './page.module.css'

export default function NotFound() {
  return (
    <main className={styles.page}>
      <section className={styles.finalCta}>
        <p className={styles.kicker}>404</p>
        <h1 className={styles.title}>This track is empty.</h1>
        <p>Head back to the StashTrack landing page.</p>
        <Link className={styles.primaryButton} href="/">
          Back to StashTrack
        </Link>
      </section>
    </main>
  )
}
