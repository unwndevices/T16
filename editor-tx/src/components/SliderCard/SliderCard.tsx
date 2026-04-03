import { Card, Slider } from '@/design-system'
import styles from './SliderCard.module.css'

interface SliderCardProps {
  label: string
  value: number
  onValueChange: (value: number) => void
  min: number
  max: number
  step?: number
  disabled?: boolean
  showSyncDot?: boolean
}

export function SliderCard({
  label,
  value,
  onValueChange,
  min,
  max,
  step = 1,
  disabled = false,
  showSyncDot = false,
}: SliderCardProps) {
  return (
    <Card>
      <div className={styles.row}>
        <div className={styles.labelGroup}>
          {showSyncDot ? <span className={styles.syncDot} /> : null}
          <span className={styles.label}>{label}</span>
        </div>
        <span className={styles.value}>{value}</span>
      </div>
      <Slider
        value={value}
        onValueChange={onValueChange}
        min={min}
        max={max}
        step={step}
        disabled={disabled}
      />
    </Card>
  )
}
