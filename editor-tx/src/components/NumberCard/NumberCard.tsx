import { Card, Button } from '@/design-system'
import styles from './NumberCard.module.css'

interface NumberCardProps {
  label: string
  value: number
  onChange: (value: number) => void
  min: number
  max: number
  disabled?: boolean
  showSyncDot?: boolean
}

export function NumberCard({
  label,
  value,
  onChange,
  min,
  max,
  disabled = false,
  showSyncDot = false,
}: NumberCardProps) {
  const handleDecrement = () => {
    if (value > min) onChange(value - 1)
  }

  const handleIncrement = () => {
    if (value < max) onChange(value + 1)
  }

  return (
    <Card>
      <div className={styles.row}>
        <div className={styles.labelGroup}>
          {showSyncDot ? <span className={styles.syncDot} /> : null}
          <span className={styles.label}>{label}</span>
        </div>
        <div className={styles.controls}>
          <Button
            variant="ghost"
            size="sm"
            onClick={handleDecrement}
            disabled={disabled || value <= min}
            aria-label={`Decrease ${label}`}
          >
            -
          </Button>
          <span className={styles.value}>{value}</span>
          <Button
            variant="ghost"
            size="sm"
            onClick={handleIncrement}
            disabled={disabled || value >= max}
            aria-label={`Increase ${label}`}
          >
            +
          </Button>
        </div>
      </div>
    </Card>
  )
}
