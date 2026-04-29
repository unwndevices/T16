import { useEffect, useRef } from 'react'
import {
  Dialog,
  DialogContent,
  DialogTitle,
  DialogDescription,
  Button,
} from '@/design-system'
import type { Variant } from '@/types/variant'
import styles from './FlashOverrideDialog.module.css'

export type FlashOverrideDialogProps = {
  open: boolean
  deviceVariant: Variant
  targetVariant: Variant
  onConfirm: () => void
  onCancel: () => void
}

export function FlashOverrideDialog({
  open,
  deviceVariant,
  targetVariant,
  onConfirm,
  onCancel,
}: FlashOverrideDialogProps) {
  const cancelRef = useRef<HTMLButtonElement | null>(null)
  useEffect(() => {
    if (open) {
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
        <DialogTitle>Flash a different variant?</DialogTitle>
        <DialogDescription>
          {`Your connected device reports ${deviceVariant}. You selected ${targetVariant}. Flashing the wrong variant will require a recalibration and may render keys non-functional until the matching firmware is reflashed.`}
        </DialogDescription>
        <div className={styles.actions}>
          <Button variant="secondary" onClick={onCancel} ref={cancelRef}>
            {`Keep ${deviceVariant}`}
          </Button>
          <Button
            variant="primary"
            className={styles.destructive}
            onClick={onConfirm}
          >
            {`Flash ${targetVariant} anyway`}
          </Button>
        </div>
      </DialogContent>
    </Dialog>
  )
}
