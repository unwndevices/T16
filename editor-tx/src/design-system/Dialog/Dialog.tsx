import { Dialog as RadixDialog } from 'radix-ui'
import styles from './Dialog.module.css'

interface DialogProps {
  open?: boolean
  onOpenChange?: (open: boolean) => void
  children: React.ReactNode
}

export function Dialog({ children, ...props }: DialogProps) {
  return <RadixDialog.Root {...props}>{children}</RadixDialog.Root>
}

interface DialogTriggerProps {
  children: React.ReactNode
  asChild?: boolean
}

export function DialogTrigger({ children, asChild = true }: DialogTriggerProps) {
  return <RadixDialog.Trigger asChild={asChild}>{children}</RadixDialog.Trigger>
}

interface DialogContentProps {
  children: React.ReactNode
  className?: string
}

export function DialogContent({ children, className }: DialogContentProps) {
  const classNames = [styles.content, className].filter(Boolean).join(' ')

  return (
    <RadixDialog.Portal>
      <RadixDialog.Overlay className={styles.overlay} />
      <RadixDialog.Content className={classNames}>{children}</RadixDialog.Content>
    </RadixDialog.Portal>
  )
}

interface DialogTitleProps {
  children: React.ReactNode
}

export function DialogTitle({ children }: DialogTitleProps) {
  return <RadixDialog.Title className={styles.title}>{children}</RadixDialog.Title>
}

interface DialogDescriptionProps {
  children: React.ReactNode
}

export function DialogDescription({ children }: DialogDescriptionProps) {
  return (
    <RadixDialog.Description className={styles.description}>{children}</RadixDialog.Description>
  )
}

interface DialogCloseProps {
  children: React.ReactNode
  asChild?: boolean
}

export function DialogClose({ children, asChild = true }: DialogCloseProps) {
  return <RadixDialog.Close asChild={asChild}>{children}</RadixDialog.Close>
}
