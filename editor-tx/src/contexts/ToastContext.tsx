import { createContext, useState, useCallback, useRef } from 'react'
import type { ReactNode } from 'react'
import { Toast } from 'radix-ui'
import '@/styles/toast.css'

export interface ToastOptions {
  title: string
  description?: string
  variant?: 'default' | 'error' | 'success'
}

interface ToastItem extends ToastOptions {
  id: number
  open: boolean
}

export interface ToastContextValue {
  toast(options: ToastOptions): void
}

const ToastContext = createContext<ToastContextValue | null>(null)

const AUTO_DISMISS_MS = 5000

interface ToastProviderProps {
  children: ReactNode
}

export function ToastProvider({ children }: ToastProviderProps) {
  const [toasts, setToasts] = useState<ToastItem[]>([])
  const nextIdRef = useRef(0)

  const toast = useCallback((options: ToastOptions) => {
    const id = nextIdRef.current++
    const newToast: ToastItem = {
      ...options,
      id,
      open: true,
    }

    setToasts((prev) => [...prev, newToast])
  }, [])

  const handleOpenChange = useCallback((id: number, open: boolean) => {
    if (!open) {
      setToasts((prev) => prev.filter((t) => t.id !== id))
    }
  }, [])

  const value: ToastContextValue = { toast }

  return (
    <ToastContext.Provider value={value}>
      <Toast.Provider duration={AUTO_DISMISS_MS}>
        {children}
        {toasts.map((t) => (
          <Toast.Root
            key={t.id}
            open={t.open}
            onOpenChange={(open) => handleOpenChange(t.id, open)}
            className={`toast-root toast-${t.variant ?? 'default'}`}
          >
            <Toast.Title className="toast-title">{t.title}</Toast.Title>
            {t.description && (
              <Toast.Description className="toast-description">{t.description}</Toast.Description>
            )}
            <Toast.Close className="toast-close" aria-label="Close">
              x
            </Toast.Close>
          </Toast.Root>
        ))}
        <Toast.Viewport className="toast-viewport" />
      </Toast.Provider>
    </ToastContext.Provider>
  )
}

export default ToastContext
