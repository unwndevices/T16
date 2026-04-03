import { Select as RadixSelect } from 'radix-ui'
import styles from './Select.module.css'

interface SelectOption {
  value: string
  label: string
}

interface SelectProps {
  label?: string
  value: string
  onValueChange: (value: string) => void
  options: SelectOption[]
  disabled?: boolean
  placeholder?: string
}

export function Select({
  label,
  value,
  onValueChange,
  options,
  disabled = false,
  placeholder = 'Select...',
}: SelectProps) {
  return (
    <div className={styles.wrapper}>
      {label ? <label className={styles.label}>{label}</label> : null}
      <RadixSelect.Root
        value={value}
        onValueChange={onValueChange}
        disabled={disabled}
      >
        <RadixSelect.Trigger className={styles.trigger}>
          <RadixSelect.Value placeholder={placeholder} />
          <RadixSelect.Icon className={styles.icon}>
            <ChevronIcon />
          </RadixSelect.Icon>
        </RadixSelect.Trigger>

        <RadixSelect.Portal>
          <RadixSelect.Content className={styles.content} position="popper" sideOffset={4}>
            <RadixSelect.Viewport className={styles.viewport}>
              {options.map((option) => (
                <RadixSelect.Item
                  key={option.value}
                  value={option.value}
                  className={styles.item}
                >
                  <RadixSelect.ItemText>{option.label}</RadixSelect.ItemText>
                  <RadixSelect.ItemIndicator className={styles.indicator}>
                    <CheckIcon />
                  </RadixSelect.ItemIndicator>
                </RadixSelect.Item>
              ))}
            </RadixSelect.Viewport>
          </RadixSelect.Content>
        </RadixSelect.Portal>
      </RadixSelect.Root>
    </div>
  )
}

function ChevronIcon() {
  return (
    <svg width="12" height="12" viewBox="0 0 12 12" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M3 4.5L6 7.5L9 4.5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  )
}

function CheckIcon() {
  return (
    <svg width="12" height="12" viewBox="0 0 12 12" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M2.5 6L5 8.5L9.5 3.5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  )
}
