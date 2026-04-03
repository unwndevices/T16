import { Toast as RadixToast } from 'radix-ui'
import styles from './Toast.module.css'

type ToastVariant = 'default' | 'error' | 'success'

export function ToastProvider({ children }: { children: React.ReactNode }) {
  return <RadixToast.Provider swipeDirection="right">{children}</RadixToast.Provider>
}

export function ToastViewport() {
  return <RadixToast.Viewport className={styles.viewport} />
}

interface ToastProps {
  open?: boolean
  onOpenChange?: (open: boolean) => void
  variant?: ToastVariant
  children: React.ReactNode
}

export function Toast({
  variant = 'default',
  children,
  ...props
}: ToastProps) {
  const classNames = [styles.toast, styles[variant]].join(' ')

  return (
    <RadixToast.Root className={classNames} {...props}>
      {children}
    </RadixToast.Root>
  )
}

interface ToastTitleProps {
  children: React.ReactNode
}

export function ToastTitle({ children }: ToastTitleProps) {
  return (
    <RadixToast.Title className={styles.title}>
      {children}
    </RadixToast.Title>
  )
}

interface ToastDescriptionProps {
  children: React.ReactNode
}

export function ToastDescription({ children }: ToastDescriptionProps) {
  return (
    <RadixToast.Description className={styles.description}>
      {children}
    </RadixToast.Description>
  )
}

interface ToastCloseProps {
  children?: React.ReactNode
}

export function ToastClose({ children }: ToastCloseProps) {
  return (
    <RadixToast.Close className={styles.close}>
      {children ?? <CloseIcon />}
    </RadixToast.Close>
  )
}

function CloseIcon() {
  return (
    <svg width="12" height="12" viewBox="0 0 12 12" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M3 3L9 9M9 3L3 9" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  )
}
