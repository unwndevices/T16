import { useState, useEffect, useCallback } from 'react'
import { Button, Card } from '@/design-system'
import { useToast } from '@/hooks/useToast'
import { useBootloader } from '@/hooks/useBootloader'
import type { BootloaderState } from '@/hooks/useBootloader'
import releaseNotes from '@/assets/firmwares/release_notes.json'
import styles from './Upload.module.css'

interface FirmwareRelease {
  version: string
  fileName: string
  releaseDate: string
  highlights: string
  bugfixes: string[]
  improvements: string[]
}

interface ReleaseNotesMap {
  [version: string]: {
    fileName: string
    version: number
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

function getButtonLabel(state: BootloaderState, isMidiConnected: boolean): string {
  switch (state) {
    case 'idle':
      return isMidiConnected ? 'Update Firmware' : 'Connect Device'
    case 'entering_bootloader':
      return 'Entering Update Mode...'
    case 'bootloader_ready':
      return 'Upload Firmware'
    case 'uploading':
      return 'Uploading...'
    case 'success':
    case 'error':
      return 'Update Firmware'
  }
}

function getButtonVariant(state: BootloaderState, isMidiConnected: boolean): 'primary' | 'secondary' {
  if (state === 'idle' && !isMidiConnected) return 'secondary'
  return 'primary'
}

function isButtonDisabled(state: BootloaderState): boolean {
  return state === 'entering_bootloader' || state === 'uploading'
}

export function Upload() {
  const [firmwares, setFirmwares] = useState<FirmwareRelease[]>([])
  const [selectedFirmware, setSelectedFirmware] = useState<FirmwareRelease | null>(null)
  const { state, progress, enterBootloader, uploadFirmware, reset, isConnected: isMidiConnected } =
    useBootloader()
  const { toast } = useToast()

  useEffect(() => {
    const notes = releaseNotes as unknown as ReleaseNotesMap
    const parsed: FirmwareRelease[] = Object.entries(notes).map(([version, data]) => ({
      version,
      fileName: data.fileName,
      releaseDate: data.releaseDate,
      highlights: data.releaseNotes.Highlights,
      bugfixes: data.releaseNotes.Bugfixes ?? [],
      improvements: data.releaseNotes.Improvements ?? [],
    }))
    setFirmwares(parsed)
    if (parsed.length > 0) {
      setSelectedFirmware(parsed[0])
    }
  }, [])

  const handleFirmwareChange = (event: React.ChangeEvent<HTMLSelectElement>) => {
    const selected = firmwares.find((fw) => fw.version === event.target.value)
    setSelectedFirmware(selected ?? null)

    // Downgrade warning: compare selected version number with device version if available
    if (selected) {
      const notes = releaseNotes as unknown as ReleaseNotesMap
      const selectedVersionNum = notes[selected.version]?.version
      // If the first firmware is the latest, check if selected is older
      const latestVersionNum = firmwares.length > 0 ? notes[firmwares[0].version]?.version : undefined
      if (
        selectedVersionNum !== undefined &&
        latestVersionNum !== undefined &&
        selectedVersionNum < latestVersionNum
      ) {
        toast({
          title: 'You are installing an older firmware version. Your settings will be preserved.',
          variant: 'default',
        })
      }
    }
  }

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
      const firmwareUrl = new URL(
        `../../assets/firmwares/${selectedFirmware.fileName}`,
        import.meta.url,
      ).href
      await uploadFirmware(firmwareUrl)
    }
  }, [state, isMidiConnected, enterBootloader, uploadFirmware, reset, selectedFirmware])

  return (
    <div className={styles.page}>
      <h1 className={styles.title}>Firmware Update</h1>

      <Card>
        <div className={styles.instructions}>
          <h3 className={styles.instructionsTitle}>How to update</h3>
          <ol className={styles.instructionsList}>
            <li>Select a firmware version below.</li>
            <li>
              Click <strong>Update Firmware</strong>. Your T16 will restart into update mode
              automatically.
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
          <label className={styles.label} htmlFor="firmware-select">
            Firmware Version
          </label>
          <select
            id="firmware-select"
            className={styles.select}
            onChange={handleFirmwareChange}
            value={selectedFirmware?.version ?? ''}
          >
            <option value="" disabled>
              Select firmware version
            </option>
            {firmwares.map((fw) => (
              <option key={fw.version} value={fw.version}>
                {fw.version} - {fw.releaseDate}
              </option>
            ))}
          </select>
        </div>

        <Button
          onClick={handleAction}
          variant={getButtonVariant(state, isMidiConnected)}
          disabled={isButtonDisabled(state)}
        >
          {getButtonLabel(state, isMidiConnected)}
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
    </div>
  )
}
