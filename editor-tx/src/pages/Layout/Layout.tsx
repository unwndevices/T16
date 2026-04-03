import { Outlet } from 'react-router'
import { NavBar } from '@/components/NavBar/NavBar'
import { Footer } from '@/components/Footer/Footer'
import styles from './Layout.module.css'

export function Layout() {
  return (
    <div className={styles.root}>
      <NavBar />
      <main className={styles.content}>
        <Outlet />
      </main>
      <Footer />
    </div>
  )
}
