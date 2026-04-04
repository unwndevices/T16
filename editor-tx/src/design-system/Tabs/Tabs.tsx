import { Tabs as RadixTabs } from 'radix-ui'
import styles from './Tabs.module.css'

interface TabsProps {
  defaultValue?: string
  value?: string
  onValueChange?: (value: string) => void
  children: React.ReactNode
}

export function Tabs({ children, ...props }: TabsProps) {
  return (
    <RadixTabs.Root className={styles.root} {...props}>
      {children}
    </RadixTabs.Root>
  )
}

interface TabsListProps {
  children: React.ReactNode
}

export function TabsList({ children }: TabsListProps) {
  return <RadixTabs.List className={styles.list}>{children}</RadixTabs.List>
}

interface TabsTriggerProps {
  value: string
  children: React.ReactNode
  disabled?: boolean
}

export function TabsTrigger({ value, children, disabled }: TabsTriggerProps) {
  return (
    <RadixTabs.Trigger className={styles.trigger} value={value} disabled={disabled}>
      {children}
    </RadixTabs.Trigger>
  )
}

interface TabsContentProps {
  value: string
  children: React.ReactNode
}

export function TabsContent({ value, children }: TabsContentProps) {
  return (
    <RadixTabs.Content className={styles.content} value={value}>
      {children}
    </RadixTabs.Content>
  )
}
