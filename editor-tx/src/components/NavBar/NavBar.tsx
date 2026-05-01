import { useState, useEffect } from 'react'
import { NavLink } from 'react-router'
import { MdTune, MdSystemUpdate, MdEqualizer, MdMenuBook, MdUsb, MdBluetooth } from 'react-icons/md'
import { useConnection } from '@/hooks/useConnection'
import { Button } from '@/design-system'
import { Tooltip, TooltipTrigger, TooltipContent } from '@/design-system'
import { SyncIndicator } from '@/components/SyncIndicator'
import { VariantIndicator } from '@/components/VariantIndicator'
import styles from './NavBar.module.css'

export function NavBar() {
  const { isConnected, connect, connectBLE, disconnect } = useConnection()

  const [isOffline, setIsOffline] = useState(!navigator.onLine)
  useEffect(() => {
    const handleOnline = () => setIsOffline(false)
    const handleOffline = () => setIsOffline(true)
    window.addEventListener('online', handleOnline)
    window.addEventListener('offline', handleOffline)
    return () => {
      window.removeEventListener('online', handleOnline)
      window.removeEventListener('offline', handleOffline)
    }
  }, [])

  return (
    <>
      <nav className={styles.nav}>
        <div className={styles.inner}>
          <div className={styles.links}>
            <Tooltip>
              <TooltipTrigger>
                <NavLink
                  to="/"
                  className={({ isActive }) => `${styles.navLink} ${isActive ? styles.active : ''}`}
                  aria-label="Editor"
                >
                  <MdTune size={22} />
                </NavLink>
              </TooltipTrigger>
              <TooltipContent side="bottom">Editor</TooltipContent>
            </Tooltip>

            <Tooltip>
              <TooltipTrigger>
                <NavLink
                  to="/upload"
                  className={({ isActive }) => `${styles.navLink} ${isActive ? styles.active : ''}`}
                  aria-label="Update"
                >
                  <MdSystemUpdate size={22} />
                </NavLink>
              </TooltipTrigger>
              <TooltipContent side="bottom">Update</TooltipContent>
            </Tooltip>

            <Tooltip>
              <TooltipTrigger>
                <NavLink
                  to="/monitor"
                  className={({ isActive }) => `${styles.navLink} ${isActive ? styles.active : ''}`}
                  aria-label="Monitor"
                >
                  <MdEqualizer size={22} />
                </NavLink>
              </TooltipTrigger>
              <TooltipContent side="bottom">Monitor</TooltipContent>
            </Tooltip>

            <Tooltip>
              <TooltipTrigger>
                <NavLink
                  to="/manual"
                  className={({ isActive }) => `${styles.navLink} ${isActive ? styles.active : ''}`}
                  aria-label="Manual"
                >
                  <MdMenuBook size={22} />
                </NavLink>
              </TooltipTrigger>
              <TooltipContent side="bottom">Manual</TooltipContent>
            </Tooltip>
          </div>

          <h1 className={styles.title}>T16 Editor</h1>

          <div className={styles.actions}>
            <SyncIndicator />
            <VariantIndicator />
            {isConnected ? (
              <Button variant="secondary" size="sm" onClick={disconnect}>
                Disconnect
              </Button>
            ) : (
              <div className={styles.connectGroup}>
                <Button variant="primary" size="sm" onClick={connect}>
                  <MdUsb size={16} /> USB
                </Button>
                <Button variant="secondary" size="sm" onClick={connectBLE}>
                  <MdBluetooth size={16} /> BLE
                </Button>
              </div>
            )}
          </div>
        </div>
      </nav>
      {isOffline && (
        <div className={styles.offlineBanner} role="status" aria-live="polite">
          You're offline. Device communication requires a connection.
        </div>
      )}
    </>
  )
}
