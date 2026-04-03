import { useConnection } from '@/hooks/useConnection'
import { useConfig } from '@/hooks/useConfig'
import styles from './SyncIndicator.module.css'

export function SyncIndicator() {
  const { isConnected } = useConnection()
  const { isSynced } = useConfig()

  let statusClass = styles.disconnected
  let label = 'Disconnected'

  if (isConnected && isSynced) {
    statusClass = styles.synced
    label = 'Synced'
  } else if (isConnected) {
    statusClass = styles.unsynced
    label = 'Unsynced'
  }

  return (
    <span
      className={`${styles.dot} ${statusClass}`}
      role="status"
      aria-label={label}
      title={label}
    />
  )
}
