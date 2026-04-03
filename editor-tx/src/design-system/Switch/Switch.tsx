import { Switch as RadixSwitch } from 'radix-ui'
import styles from './Switch.module.css'

interface SwitchProps {
  label?: string
  checked: boolean
  onCheckedChange: (checked: boolean) => void
  disabled?: boolean
}

export function Switch({
  label,
  checked,
  onCheckedChange,
  disabled = false,
}: SwitchProps) {
  return (
    <div className={styles.wrapper}>
      <RadixSwitch.Root
        className={styles.root}
        checked={checked}
        onCheckedChange={onCheckedChange}
        disabled={disabled}
      >
        <RadixSwitch.Thumb className={styles.thumb} />
      </RadixSwitch.Root>
      {label ? <label className={styles.label}>{label}</label> : null}
    </div>
  )
}
