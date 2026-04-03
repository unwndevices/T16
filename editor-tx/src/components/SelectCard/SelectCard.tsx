import { Card, Select } from '@/design-system'
import styles from './SelectCard.module.css'

interface SelectOption {
  value: string
  label: string
}

interface SelectCardProps {
  label: string
  value: string
  onValueChange: (value: string) => void
  options: SelectOption[]
  disabled?: boolean
  showSyncDot?: boolean
}

export function SelectCard({
  label,
  value,
  onValueChange,
  options,
  disabled = false,
  showSyncDot = false,
}: SelectCardProps) {
  return (
    <Card>
      <div className={styles.row}>
        <div className={styles.labelGroup}>
          {showSyncDot ? <span className={styles.syncDot} /> : null}
          <span className={styles.label}>{label}</span>
        </div>
        <div className={styles.control}>
          <Select
            value={value}
            onValueChange={onValueChange}
            options={options}
            disabled={disabled}
          />
        </div>
      </div>
    </Card>
  )
}
