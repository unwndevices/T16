import { Outlet } from 'react-router'
import { NavBar } from '@/components/NavBar/NavBar'
import { Footer } from '@/components/Footer/Footer'
import { CrossVariantAdaptDialog } from '@/components/CrossVariantAdaptDialog'
import { useConfig } from '@/hooks/useConfig'
import { useToast } from '@/hooks/useToast'
import { adaptConfigForVariant } from '@/services/adaptConfigForVariant'
import styles from './Layout.module.css'

export function Layout() {
  const { pendingAdaptation, confirmAdaptation, cancelAdaptation } = useConfig()
  const { toast } = useToast()

  const handleAdapt = () => {
    if (!pendingAdaptation) return
    const { fileConfig, fileVariant, deviceVariant } = pendingAdaptation
    confirmAdaptation()
    // WR-03: describe what adaptConfigForVariant ACTUALLY did, not what a
    // future v202+ adapt is expected to do. For v201 the adapt is a pure
    // variant rewrite — no per-key arrays exist to pad/truncate. Emitting a
    // destructive-sounding "keys 17–32 discarded" toast for a benign rewrite
    // misleads the user. Compare input vs output to pick the right copy.
    const adapted = adaptConfigForVariant(fileConfig, deviceVariant)
    const onlyVariantChanged =
      JSON.stringify({ ...fileConfig, variant: deviceVariant }) === JSON.stringify(adapted)
    const toastMsg = onlyVariantChanged
      ? `Config variant rewritten: ${fileVariant} → ${deviceVariant} (no per-key data changed in v${fileConfig.version}).`
      : fileVariant === 'T16' && deviceVariant === 'T32'
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
