import { useState, useEffect, useCallback, useRef, useMemo } from 'react'
import { Button, Card, Select } from '@/design-system'
import { useToast } from '@/hooks/useToast'
import { useBootloader } from '@/hooks/useBootloader'
import type { BootloaderState } from '@/hooks/useBootloader'
import { useVariant } from '@/hooks/useVariant'
import type { Variant } from '@/types/variant'
import releaseNotes from '@/assets/firmwares/release_notes.json'
import { FlashOverrideDialog } from './FlashOverrideDialog'
import styles from './Upload.module.css'

interface FirmwareRelease {
  version: string
  fileName: string
  variant: Variant
  unavailable: boolean
  releaseDate: string
  highlights: string
  bugfixes: string[]
  improvements: string[]
}

interface ReleaseNotesMap {
  [version: string]: {
    fileName: string
    version: number
    variant?: Variant
    unavailable?: boolean
    releaseDate: string
    releaseNotes: {
      Highlights: string
      Bugfixes?: string[]
      Improvements?: string[]
    }
  }
}

function ReleaseNotesCard({ firmware }: { firmware: FirmwareRelease | null }) {
  if (!firmware) return null

  return (
    <Card title={`Release Notes - ${firmware.version}`}>
      {firmware.highlights && <p className={styles.highlights}>{firmware.highlights}</p>}
      {firmware.bugfixes && firmware.bugfixes.length > 0 && (
        <div className={styles.notesSection}>
          <h4 className={styles.notesSectionTitle}>Bugfixes</h4>
          <ul className={styles.notesList}>
            {firmware.bugfixes.map((fix, index) => (
              <li key={index}>{fix}</li>
            ))}
          </ul>
        </div>
      )}
      {firmware.improvements && firmware.improvements.length > 0 && (
        <div className={styles.notesSection}>
          <h4 className={styles.notesSectionTitle}>Improvements</h4>
          <ul className={styles.notesList}>
            {firmware.improvements.map((improvement, index) => (
              <li key={index}>{improvement}</li>
            ))}
          </ul>
        </div>
      )}
    </Card>
  )
}

function getButtonLabel(
  state: BootloaderState,
  isMidiConnected: boolean,
  selectedVariant: Variant,
): string {
  switch (state) {
    case 'idle':
      return isMidiConnected ? `Flash ${selectedVariant} firmware` : 'Connect Device'
    case 'entering_bootloader':
      return 'Entering Update Mode...'
    case 'bootloader_ready':
      return `Flash ${selectedVariant} firmware`
    case 'uploading':
      return 'Uploading...'
    case 'success':
    case 'error':
      return `Flash ${selectedVariant} firmware`
  }
}

function getButtonVariant(
  state: BootloaderState,
  isMidiConnected: boolean,
): 'primary' | 'secondary' {
  if (state === 'idle' && !isMidiConnected) return 'secondary'
  return 'primary'
}

function isButtonDisabled(state: BootloaderState): boolean {
  return state === 'entering_bootloader' || state === 'uploading'
}

// Firmware binaries are resolved at flash time via `new URL(...)` against
// the assets/firmwares directory (see flashSelectedFirmware below). Vite
// emits hashed asset files for each filename it can statically analyze
// from the `${selectedFirmware.fileName}` template — currently the T16
// .bin files. The T32 entry in release_notes.json is marked
// `unavailable: true` and the UI surfaces the locked "firmware unavailable
// in this build" copy when the user selects T32, so the placeholder .bin
// does NOT need to ship in the bundle yet. When Phase 12 hardware
// bring-up produces a real T32 firmware tag, drop `unavailable: true`
// from release_notes.json and Vite will pick it up automatically.

// Parse the firmware release notes once at module load — it's a static JSON asset.
const ALL_FIRMWARES: FirmwareRelease[] = Object.entries(
  releaseNotes as unknown as ReleaseNotesMap,
).map(([version, data]) => ({
  version,
  fileName: data.fileName,
  variant: data.variant ?? 'T16',
  unavailable: data.unavailable === true,
  releaseDate: data.releaseDate,
  highlights: data.releaseNotes.Highlights,
  bugfixes: data.releaseNotes.Bugfixes ?? [],
  improvements: data.releaseNotes.Improvements ?? [],
}))

export function Upload() {
  const { variant: deviceVariant, isHandshakeConfirmed } = useVariant()
  const [selectedVariant, setSelectedVariant] = useState<Variant>(
    isHandshakeConfirmed ? deviceVariant : 'T16',
  )
  const [overrideOpen, setOverrideOpen] = useState(false)
  const userPickedRef = useRef(false)
  const handshakeAppliedRef = useRef(false)
  const {
    state,
    progress,
    enterBootloader,
    uploadFirmware,
    reset,
    isConnected: isMidiConnected,
  } = useBootloader()
  const { toast } = useToast()

  // Sync selectedVariant with handshake the first time a confirmed variant arrives —
  // but only if the user hasn't manually picked.
  useEffect(() => {
    if (
      isHandshakeConfirmed &&
      !userPickedRef.current &&
      !handshakeAppliedRef.current &&
      selectedVariant !== deviceVariant
    ) {
      handshakeAppliedRef.current = true
      setSelectedVariant(deviceVariant)
    }
  }, [isHandshakeConfirmed, deviceVariant, selectedVariant])

  // Filter firmwares by selected variant; pick the latest available (or first) on change.
  const variantFirmwares = useMemo(
    () => ALL_FIRMWARES.filter((f) => f.variant === selectedVariant),
    [selectedVariant],
  )
  const noFirmwareForVariant = variantFirmwares.length === 0
  // Only count truly available (non-placeholder) firmwares as flashable.
  const availableFirmwares = useMemo(
    () => variantFirmwares.filter((f) => !f.unavailable),
    [variantFirmwares],
  )
  const variantUnavailable = variantFirmwares.length > 0 && availableFirmwares.length === 0

  // selectedFirmware derives from availableFirmwares — first available, or null.
  // Held in state so the user can override via the version dropdown.
  const [selectedFirmwareVersion, setSelectedFirmwareVersion] = useState<string | null>(
    availableFirmwares[0]?.version ?? null,
  )
  const selectedFirmware =
    availableFirmwares.find((f) => f.version === selectedFirmwareVersion) ??
    availableFirmwares[0] ??
    null
  // Reset the per-variant version selection when the variant changes.
  useEffect(() => {
    setSelectedFirmwareVersion(availableFirmwares[0]?.version ?? null)
    // selectedVariant is the only intentional trigger; availableFirmwares dep would
    // cause a loop on every variantFirmwares re-derivation.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [selectedVariant])

  const variantOptions = useMemo(
    () => [
      {
        value: 'T16',
        label: isHandshakeConfirmed && deviceVariant === 'T16' ? 'T16 (detected)' : 'T16',
      },
      {
        value: 'T32',
        label: isHandshakeConfirmed && deviceVariant === 'T32' ? 'T32 (detected)' : 'T32',
      },
    ],
    [isHandshakeConfirmed, deviceVariant],
  )

  const flashSelectedFirmware = useCallback(async () => {
    if (!selectedFirmware || selectedFirmware.unavailable) return
    const firmwareUrl = new URL(
      `../../assets/firmwares/${selectedFirmware.fileName}`,
      import.meta.url,
    ).href
    await uploadFirmware(firmwareUrl)
  }, [selectedFirmware, uploadFirmware])

  const handleAction = useCallback(async () => {
    if (state === 'success' || state === 'error') {
      reset()
      return
    }

    if (state === 'idle' && !isMidiConnected) {
      // No MIDI connected -- this is a no-op, the user needs to connect via Dashboard first
      return
    }

    if (state === 'idle' && isMidiConnected) {
      await enterBootloader()
      return
    }

    if (state === 'bootloader_ready' && selectedFirmware) {
      if (selectedFirmware.unavailable) {
        toast({
          title: `${selectedVariant} firmware unavailable in this build. Reload the page or report this issue.`,
          variant: 'error',
        })
        return
      }
      // Phase 14-05: variant override gate. Fires only when handshake-confirmed AND
      // selectedVariant differs from deviceVariant. Offline flashing skips the modal.
      if (isHandshakeConfirmed && selectedVariant !== deviceVariant) {
        setOverrideOpen(true)
        return
      }
      await flashSelectedFirmware()
    }
  }, [
    state,
    isMidiConnected,
    enterBootloader,
    reset,
    selectedFirmware,
    isHandshakeConfirmed,
    selectedVariant,
    deviceVariant,
    flashSelectedFirmware,
    toast,
  ])

  return (
    <div className={styles.page}>
      <h1 className={styles.title}>Firmware Update</h1>

      <Card>
        <div className={styles.instructions}>
          <h3 className={styles.instructionsTitle}>How to update</h3>
          <ol className={styles.instructionsList}>
            <li>Select a firmware version below.</li>
            <li>
              Click <strong>Flash {selectedVariant} firmware</strong>. Your device will restart into
              update mode automatically.
            </li>
            <li>Wait for the upload to complete. The device will restart when done.</li>
          </ol>
          <p className={styles.fallbackLink}>
            Device not entering update mode? Hold the BOOT button while plugging in.
          </p>
        </div>
      </Card>

      <div className={styles.controls}>
        <div className={styles.firmwareSelect}>
          <label className={styles.label}>Firmware variant</label>
          <Select
            value={selectedVariant}
            onValueChange={(v) => {
              userPickedRef.current = true
              setSelectedVariant(v as Variant)
            }}
            options={variantOptions}
          />
        </div>

        {(noFirmwareForVariant || variantUnavailable) && (
          <p className={styles.warning}>
            {`${selectedVariant} firmware unavailable in this build. Reload the page or report this issue.`}
          </p>
        )}

        {!noFirmwareForVariant && !variantUnavailable && (
          <div className={styles.firmwareSelect}>
            <label className={styles.label} htmlFor="firmware-select">
              Firmware Version
            </label>
            <select
              id="firmware-select"
              className={styles.select}
              onChange={(event) => {
                setSelectedFirmwareVersion(event.target.value || null)
              }}
              value={selectedFirmware?.version ?? ''}
            >
              <option value="" disabled>
                Select firmware version
              </option>
              {availableFirmwares.map((fw) => (
                <option key={fw.version} value={fw.version}>
                  {fw.version} - {fw.releaseDate}
                </option>
              ))}
            </select>
          </div>
        )}

        <Button
          onClick={handleAction}
          variant={getButtonVariant(state, isMidiConnected)}
          disabled={isButtonDisabled(state) || variantUnavailable || noFirmwareForVariant}
        >
          {getButtonLabel(state, isMidiConnected, selectedVariant)}
        </Button>

        {state === 'uploading' && (
          <div className={styles.progressContainer}>
            <div className={styles.progressBar}>
              <div className={styles.progressFill} style={{ width: `${progress}%` }} />
            </div>
            <span className={styles.progressText}>{progress}%</span>
          </div>
        )}
      </div>

      <ReleaseNotesCard firmware={selectedFirmware} />

      <FlashOverrideDialog
        open={overrideOpen}
        deviceVariant={deviceVariant}
        targetVariant={selectedVariant}
        onConfirm={async () => {
          setOverrideOpen(false)
          await flashSelectedFirmware()
        }}
        onCancel={() => setOverrideOpen(false)}
      />
    </div>
  )
}
