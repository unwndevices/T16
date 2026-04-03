import { useConfig } from '@/hooks/useConfig'
import styles from './BankSelector.module.css'

const BANK_LABELS = ['A', 'B', 'C', 'D'] as const

export function BankSelector() {
  const { selectedBank, setBank } = useConfig()

  return (
    <div className={styles.wrapper}>
      <span className={styles.label}>Bank</span>
      <div className={styles.buttons}>
        {BANK_LABELS.map((label, index) => (
          <button
            key={label}
            className={`${styles.bankButton} ${selectedBank === index ? styles.active : ''}`}
            onClick={() => setBank(index)}
            aria-label={`Bank ${label}`}
            aria-pressed={selectedBank === index}
          >
            {label}
          </button>
        ))}
      </div>
    </div>
  )
}
