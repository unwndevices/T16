import { useState, useRef, useCallback, useEffect } from 'react'
import { ESPLoader } from 'esptool-js'
import { Button, Card } from '@/design-system'
import { useToast } from '@/hooks/useToast'
import { useConnection } from '@/hooks/useConnection'
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
      Bugfixes: string[]
      Improvements: string[]
    }
  }
}

function ReleaseNotesCard({ firmware }: { firmware: FirmwareRelease | null }) {
  if (!firmware) return null

  return (
    <Card title={`Release Notes - ${firmware.version}`}>
      {firmware.highlights && (
        <p className={styles.highlights}>{firmware.highlights}</p>
      )}
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

export function Upload() {
  const [firmwares, setFirmwares] = useState<FirmwareRelease[]>([])
  const [selectedFirmware, setSelectedFirmware] = useState<FirmwareRelease | null>(null)
  const [isSerialConnected, setIsSerialConnected] = useState(false)
  const [isUploading, setIsUploading] = useState(false)
  const [progress, setProgress] = useState(0)
  const espLoaderRef = useRef<ESPLoader | null>(null)
  const { toast } = useToast()
  const { isConnected: isMidiConnected } = useConnection()

  useEffect(() => {
    const notes = releaseNotes as ReleaseNotesMap
    const parsed: FirmwareRelease[] = Object.entries(notes).map(
      ([version, data]) => ({
        version,
        fileName: data.fileName,
        releaseDate: data.releaseDate,
        highlights: data.releaseNotes.Highlights,
        bugfixes: data.releaseNotes.Bugfixes,
        improvements: data.releaseNotes.Improvements,
      })
    )
    setFirmwares(parsed)
    if (parsed.length > 0) {
      setSelectedFirmware(parsed[0])
    }
  }, [])

  const handleFirmwareChange = (event: React.ChangeEvent<HTMLSelectElement>) => {
    const selected = firmwares.find((fw) => fw.version === event.target.value)
    setSelectedFirmware(selected ?? null)
  }

  const handleConnect = async () => {
    try {
      const port = await navigator.serial.requestPort()
      const esploader = new ESPLoader({
        transport: 'web-serial',
        baudrate: 921600,
        port: port,
      } as ConstructorParameters<typeof ESPLoader>[0])

      await esploader.main()
      await esploader.flashId()
      espLoaderRef.current = esploader
      setIsSerialConnected(true)
      toast('Connected successfully', 'success')
    } catch (error) {
      console.error('Connection failed:', error)
      toast('Connection failed. Check that your T16 is plugged in and no other app is using it.', 'error')
    }
  }

  const handleUpload = useCallback(async () => {
    if (!selectedFirmware) {
      toast('Please select a firmware version before uploading.', 'error')
      return
    }
    if (!espLoaderRef.current) {
      toast('Device not connected. Connect first.', 'error')
      return
    }

    setIsUploading(true)
    setProgress(0)

    try {
      const firmwareUrl = new URL(
        `../../assets/firmwares/${selectedFirmware.fileName}`,
        import.meta.url
      ).href
      const response = await fetch(firmwareUrl)

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`)
      }

      const blob = await response.blob()
      const reader = new FileReader()
      const firmwareData = await new Promise<string>((resolve, reject) => {
        reader.onload = (e) => resolve(e.target?.result as string)
        reader.onerror = (e) => reject(e)
        reader.readAsBinaryString(blob)
      })

      const fileArray = [
        {
          data: firmwareData,
          address: 0x10000,
        },
      ]

      const flashOptions = {
        fileArray: fileArray,
        flashSize: 'keep',
        eraseAll: false,
        compress: true,
        reportProgress: (_fileIndex: number, written: number, total: number) => {
          const pct = Math.floor((written / total) * 100)
          setProgress(pct)
        },
      }

      await espLoaderRef.current.writeFlash(flashOptions)
      toast('Firmware uploaded successfully', 'success')
    } catch (error) {
      console.error('Upload failed:', error)
      toast('Upload failed. Please try again.', 'error')
    } finally {
      setIsUploading(false)
      setProgress(0)
    }
  }, [selectedFirmware, toast])

  const handleConnectOrUpload = () => {
    if (!isSerialConnected) {
      handleConnect()
    } else {
      handleUpload()
    }
  }

  return (
    <div className={styles.page}>
      <h1 className={styles.title}>Firmware Update</h1>

      <Card>
        <div className={styles.instructions}>
          <h3 className={styles.instructionsTitle}>How to update</h3>
          <ol className={styles.instructionsList}>
            <li>
              Hold the <strong>MODE button</strong> down while plugging in your
              device. All LEDs will stay off during the procedure.
            </li>
            <li>
              Click <strong>"Connect Device"</strong>. A popup will appear --
              select <strong>"USB JTAG/Serial debug unit"</strong> and click{' '}
              <strong>"Connect"</strong>.
            </li>
            <li>
              Select a firmware version and click{' '}
              <strong>"Update Firmware"</strong>.
            </li>
            <li>
              Once complete, <strong>unplug</strong> the device to exit Update
              mode.
            </li>
          </ol>
        </div>
      </Card>

      {isMidiConnected && (
        <div className={styles.warning}>
          Your device is connected via MIDI. You may need to disconnect it
          before flashing firmware.
        </div>
      )}

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
          onClick={handleConnectOrUpload}
          variant={isSerialConnected ? 'primary' : 'secondary'}
          disabled={isUploading}
        >
          {isUploading
            ? 'Uploading...'
            : isSerialConnected
              ? 'Update Firmware'
              : 'Connect Device'}
        </Button>

        {isUploading && (
          <div className={styles.progressContainer}>
            <div className={styles.progressBar}>
              <div
                className={styles.progressFill}
                style={{ width: `${progress}%` }}
              />
            </div>
            <span className={styles.progressText}>{progress}%</span>
          </div>
        )}
      </div>

      <ReleaseNotesCard firmware={selectedFirmware} />
    </div>
  )
}
