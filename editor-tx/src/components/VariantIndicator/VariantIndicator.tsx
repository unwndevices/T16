import { useVariant } from '@/hooks/useVariant'
import { Tooltip, TooltipTrigger, TooltipContent } from '@/design-system/Tooltip'
import { Select } from '@/design-system'
import type { Variant } from '@/types/variant'
import { ALL_VARIANTS } from '@/types/variant'
import styles from './VariantIndicator.module.css'

const PICKER_OPTIONS = [
  { value: 'T16', label: 'T16 (16 keys, 4×4)' },
  { value: 'T32', label: 'T32 (32 keys, 4×8)' },
] as const

export function VariantIndicator() {
  const { variant, isHandshakeConfirmed, setVariant } = useVariant()

  const label = isHandshakeConfirmed ? variant : `${variant} (offline)`
  const tooltipCopy = isHandshakeConfirmed
    ? `Variant: ${variant}. Detected from device.`
    : `Variant: ${variant}. No device connected — using last selection.`

  // When the handshake is confirmed, the device wins per D14.1 — the picker is
  // disabled. When offline, clicking the chip opens a Radix Select with the two
  // variant options.
  if (isHandshakeConfirmed) {
    return (
      <Tooltip>
        <TooltipTrigger>
          <button
            type="button"
            className={`${styles.chip} ${styles.active}`}
            data-variant={variant}
            aria-label={tooltipCopy}
            disabled
            aria-disabled="true"
          >
            {label}
          </button>
        </TooltipTrigger>
        <TooltipContent>{tooltipCopy}</TooltipContent>
      </Tooltip>
    )
  }

  return (
    <div className={styles.wrapper} data-variant={variant}>
      <Tooltip>
        <TooltipTrigger>
          <span className={styles.tooltipAnchor} aria-hidden="true" />
        </TooltipTrigger>
        <TooltipContent>{tooltipCopy}</TooltipContent>
      </Tooltip>
      <Select
        label="Edit layout for"
        value={variant}
        onValueChange={(v) => {
          if (ALL_VARIANTS.includes(v as Variant)) {
            setVariant(v as Variant)
          }
        }}
        options={[...PICKER_OPTIONS]}
      />
      <p className={styles.helperText}>Connect a device to override this automatically.</p>
      <span className={styles.offlineLabel}>{label}</span>
    </div>
  )
}
