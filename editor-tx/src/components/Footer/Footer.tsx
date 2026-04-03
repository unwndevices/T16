import styles from './Footer.module.css'

export function Footer() {
  return (
    <footer className={styles.footer}>
      <div className={styles.inner}>
        <span className={styles.brand}>unwn</span>
        <span className={styles.version}>T16 Editor</span>
      </div>
    </footer>
  )
}
