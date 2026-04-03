import { Card, Select, Button } from '@/design-system'
import styles from './CcCard.module.css'

interface CcCardProps {
  index: number
  name: string
  channel: number
  ccId: number
  onChannelChange: (channel: number) => void
  onIdChange: (id: number) => void
  isDuplicate?: boolean
}

const CHANNEL_OPTIONS = Array.from({ length: 16 }, (_, i) => ({
  value: String(i + 1),
  label: String(i + 1),
}))

export function CcCard({
  name,
  channel,
  ccId,
  onChannelChange,
  onIdChange,
  isDuplicate = false,
}: CcCardProps) {
  const handleCcDecrement = () => {
    if (ccId > 0) onIdChange(ccId - 1)
  }

  const handleCcIncrement = () => {
    if (ccId < 127) onIdChange(ccId + 1)
  }

  return (
    <Card className={isDuplicate ? styles.duplicate : undefined}>
      <div className={styles.header}>
        <span className={styles.name}>{name}</span>
      </div>
      <div className={styles.fields}>
        <div className={styles.field}>
          <span className={styles.fieldLabel}>CH</span>
          <Select
            value={String(channel)}
            onValueChange={(v) => onChannelChange(Number(v))}
            options={CHANNEL_OPTIONS}
          />
        </div>
        <div className={styles.field}>
          <span className={styles.fieldLabel}>CC</span>
          <div className={styles.ccControls}>
            <Button
              variant="ghost"
              size="sm"
              onClick={handleCcDecrement}
              disabled={ccId <= 0}
              aria-label="Decrease CC"
            >
              -
            </Button>
            <span className={styles.ccValue}>{ccId}</span>
            <Button
              variant="ghost"
              size="sm"
              onClick={handleCcIncrement}
              disabled={ccId >= 127}
              aria-label="Increase CC"
            >
              +
            </Button>
          </div>
        </div>
      </div>
    </Card>
  )
}
