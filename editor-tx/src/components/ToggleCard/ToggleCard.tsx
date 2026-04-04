import { Card, Switch } from '@/design-system'
import styles from './ToggleCard.module.css'

interface ToggleCardProps {
  label: string
  checked: boolean
  onCheckedChange: (checked: boolean) => void
  disabled?: boolean
  showSyncDot?: boolean
}

export function ToggleCard({
  label,
  checked,
  onCheckedChange,
  disabled = false,
  showSyncDot = false,
}: ToggleCardProps) {
  return (
    <Card>
      <div className={styles.row}>
        <div className={styles.labelGroup}>
          {showSyncDot ? <span className={styles.syncDot} /> : null}
          <span className={styles.label}>{label}</span>
        </div>
        <Switch checked={checked} onCheckedChange={onCheckedChange} disabled={disabled} />
      </div>
    </Card>
  )
}
