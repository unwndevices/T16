import { useEffect, useRef } from 'react'
import {
  Dialog,
  DialogContent,
  DialogTitle,
  DialogDescription,
  Button,
} from '@/design-system'
import type { T16Configuration } from '@/types/config'
import type { Variant } from '@/types/variant'
import styles from './CrossVariantAdaptDialog.module.css'

export type CrossVariantAdaptDialogProps = {
  open: boolean
  fileConfig: T16Configuration
  fileVariant: Variant
  deviceVariant: Variant
  onAdapt: () => void
  onCancel: () => void
}

export function CrossVariantAdaptDialog({
  open,
  fileVariant,
  deviceVariant,
  onAdapt,
  onCancel,
}: CrossVariantAdaptDialogProps) {
  const isTruncating = fileVariant === 'T32' && deviceVariant === 'T16'
  const bodyLine2 = isTruncating
    ? 'Adapting will discard per-key settings for keys 17–32. This cannot be undone for the loaded file.'
    : 'Adapting will pad per-key settings (notes, scales, CC) from 16 to 32 keys with default values for keys 17–32. Global settings are preserved.'

  const cancelRef = useRef<HTMLButtonElement | null>(null)
  useEffect(() => {
    if (open) {
      // Defer to next tick so Radix portal can mount the dialog before we focus.
      const t = setTimeout(() => cancelRef.current?.focus(), 0)
      return () => clearTimeout(t)
    }
  }, [open])

  return (
    <Dialog
      open={open}
      onOpenChange={(o) => {
        if (!o) onCancel()
      }}
    >
      <DialogContent>
        <DialogTitle>Variant mismatch</DialogTitle>
        <DialogDescription>
          {`This config is for ${fileVariant}. You're connected to ${deviceVariant}.`}
        </DialogDescription>
        <p className={isTruncating ? styles.destructive : undefined}>{bodyLine2}</p>
        <div className={styles.actions}>
          <Button variant="secondary" onClick={onCancel} ref={cancelRef}>
            Cancel
          </Button>
          <Button variant="primary" onClick={onAdapt}>
            Adapt and load
          </Button>
        </div>
      </DialogContent>
    </Dialog>
  )
}
