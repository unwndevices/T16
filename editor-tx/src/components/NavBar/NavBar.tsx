import { NavLink } from 'react-router'
import { MdTune, MdSystemUpdate, MdMenuBook } from 'react-icons/md'
import { useConnection } from '@/hooks/useConnection'
import { Button } from '@/design-system'
import { Tooltip, TooltipTrigger, TooltipContent } from '@/design-system'
import { SyncIndicator } from '@/components/SyncIndicator'
import styles from './NavBar.module.css'

export function NavBar() {
  const { isConnected, connect, disconnect } = useConnection()

  return (
    <nav className={styles.nav}>
      <div className={styles.inner}>
        <div className={styles.links}>
          <Tooltip>
            <TooltipTrigger>
              <NavLink
                to="/"
                className={({ isActive }) =>
                  `${styles.navLink} ${isActive ? styles.active : ''}`
                }
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
                className={({ isActive }) =>
                  `${styles.navLink} ${isActive ? styles.active : ''}`
                }
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
                to="/manual"
                className={({ isActive }) =>
                  `${styles.navLink} ${isActive ? styles.active : ''}`
                }
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
          <Button
            variant={isConnected ? 'secondary' : 'primary'}
            size="sm"
            onClick={isConnected ? disconnect : connect}
          >
            {isConnected ? 'Disconnect' : 'Connect Device'}
          </Button>
        </div>
      </div>
    </nav>
  )
}
