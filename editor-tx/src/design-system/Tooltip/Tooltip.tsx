import { Tooltip as RadixTooltip } from 'radix-ui'
import styles from './Tooltip.module.css'

interface TooltipProviderProps {
  children: React.ReactNode
  delayDuration?: number
}

export function TooltipProvider({ children, delayDuration = 300 }: TooltipProviderProps) {
  return <RadixTooltip.Provider delayDuration={delayDuration}>{children}</RadixTooltip.Provider>
}

interface TooltipProps {
  children: React.ReactNode
  open?: boolean
  onOpenChange?: (open: boolean) => void
}

export function Tooltip({ children, ...props }: TooltipProps) {
  return <RadixTooltip.Root {...props}>{children}</RadixTooltip.Root>
}

interface TooltipTriggerProps {
  children: React.ReactNode
  asChild?: boolean
}

export function TooltipTrigger({ children, asChild = true }: TooltipTriggerProps) {
  return <RadixTooltip.Trigger asChild={asChild}>{children}</RadixTooltip.Trigger>
}

interface TooltipContentProps {
  children: React.ReactNode
  side?: 'top' | 'right' | 'bottom' | 'left'
  sideOffset?: number
}

export function TooltipContent({ children, side = 'top', sideOffset = 4 }: TooltipContentProps) {
  return (
    <RadixTooltip.Portal>
      <RadixTooltip.Content className={styles.content} side={side} sideOffset={sideOffset}>
        {children}
        <RadixTooltip.Arrow className={styles.arrow} />
      </RadixTooltip.Content>
    </RadixTooltip.Portal>
  )
}
