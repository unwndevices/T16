import { Card, Slider } from '@/design-system'
import { useVariant } from '@/hooks/useVariant'
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
  /** When true, gate visibility on `capabilities.touchSlider` (Phase 14-05). */
  capabilityKey?: 'touchSlider'
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
  capabilityKey,
}: SliderCardProps) {
  const { variant, capabilities, isHandshakeConfirmed } = useVariant()

  if (capabilityKey === 'touchSlider' && !capabilities.touchSlider) {
    const bodyCopy = isHandshakeConfirmed
      ? `Touch slider is hardware-specific to T16 and isn't supported on this variant.`
      : `Touch slider availability will be confirmed when a device connects.`
    return (
      <Card>
        <div className={styles.placeholder}>
          <h3 className={styles.placeholderHeading}>{`Not available on ${variant}`}</h3>
          <p className={styles.placeholderBody}>{bodyCopy}</p>
        </div>
      </Card>
    )
  }

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
