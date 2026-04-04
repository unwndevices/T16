import { Slider as RadixSlider } from 'radix-ui'
import styles from './Slider.module.css'

interface SliderProps {
  label?: string
  value: number
  onValueChange: (value: number) => void
  min?: number
  max?: number
  step?: number
  disabled?: boolean
}

export function Slider({
  label,
  value,
  onValueChange,
  min = 0,
  max = 127,
  step = 1,
  disabled = false,
}: SliderProps) {
  return (
    <div className={styles.wrapper}>
      {label ? (
        <div className={styles.header}>
          <label className={styles.label}>{label}</label>
          <span className={styles.value}>{value}</span>
        </div>
      ) : null}
      <RadixSlider.Root
        className={styles.root}
        value={[value]}
        onValueChange={([v]) => {
          if (v !== undefined) onValueChange(v)
        }}
        min={min}
        max={max}
        step={step}
        disabled={disabled}
      >
        <RadixSlider.Track className={styles.track}>
          <RadixSlider.Range className={styles.range} />
        </RadixSlider.Track>
        <RadixSlider.Thumb className={styles.thumb} />
      </RadixSlider.Root>
    </div>
  )
}
