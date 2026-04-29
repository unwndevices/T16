import { Outlet } from 'react-router'
import { NavBar } from '@/components/NavBar/NavBar'
import { Footer } from '@/components/Footer/Footer'
import { CrossVariantAdaptDialog } from '@/components/CrossVariantAdaptDialog'
import { useConfig } from '@/hooks/useConfig'
import { useToast } from '@/hooks/useToast'
import styles from './Layout.module.css'

export function Layout() {
  const { pendingAdaptation, confirmAdaptation, cancelAdaptation } = useConfig()
  const { toast } = useToast()

  const handleAdapt = () => {
    if (!pendingAdaptation) return
    const { fileVariant, deviceVariant } = pendingAdaptation
    confirmAdaptation()
    const toastMsg =
      fileVariant === 'T16' && deviceVariant === 'T32'
        ? 'Config adapted: T16 → T32 (16 keys padded with defaults).'
        : 'Config adapted: T32 → T16 (keys 17–32 discarded).'
    toast({ title: toastMsg, variant: 'success' })
  }

  return (
    <div className={styles.root}>
      <NavBar />
      <main className={styles.content}>
        <Outlet />
      </main>
      <Footer />
      {pendingAdaptation && (
        <CrossVariantAdaptDialog
          open={true}
          fileConfig={pendingAdaptation.fileConfig}
          fileVariant={pendingAdaptation.fileVariant}
          deviceVariant={pendingAdaptation.deviceVariant}
          onAdapt={handleAdapt}
          onCancel={cancelAdaptation}
        />
      )}
    </div>
  )
}
