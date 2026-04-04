import { useState, useCallback, useRef } from 'react'
import { ESPLoader, Transport } from 'esptool-js'
import { useConnection } from '@/hooks/useConnection'
import { requestBootloader } from '@/protocol/sysex'
import { useToast } from '@/hooks/useToast'

export type BootloaderState =
  | 'idle'
  | 'entering_bootloader'
  | 'bootloader_ready'
  | 'uploading'
  | 'success'
  | 'error'

export function useBootloader() {
  const [state, setState] = useState<BootloaderState>('idle')
  const [progress, setProgress] = useState(0)
  const espLoaderRef = useRef<ESPLoader | null>(null)
  const { output, isConnected } = useConnection()
  const { toast } = useToast()

  const enterBootloader = useCallback(async () => {
    if (!output || !isConnected) return

    setState('entering_bootloader')

    try {
      // Send bootloader SysEx command
      requestBootloader(output)

      // Wait for MIDI device to disconnect (ESP32 restarts into download mode)
      await new Promise<void>((resolve, reject) => {
        const timeout = setTimeout(
          () => reject(new Error('Bootloader entry timeout')),
          5000,
        )
        const check = setInterval(() => {
          if (!output.connection || output.connection === 'closed') {
            clearInterval(check)
            clearTimeout(timeout)
            resolve()
          }
        }, 100)
      })

      // Wait for USB re-enumeration after ESP32 resets into download mode
      await new Promise((resolve) => setTimeout(resolve, 1000))

      // Try to auto-connect via Web Serial (previously granted port)
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      const ports = await (navigator as any).serial.getPorts()
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      let port = ports.find((p: any) => {
        // eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call, @typescript-eslint/no-unsafe-member-access
        const info = p.getInfo()
        // ESP32-S3 USB Serial/JTAG: VID 0x303A, PID 0x1001
        // eslint-disable-next-line @typescript-eslint/no-unsafe-member-access
        return info.usbVendorId === 0x303A && info.usbProductId === 0x1001
      })

      if (!port) {
        // Fallback: prompt user to select the port
        // eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call, @typescript-eslint/no-unsafe-member-access, @typescript-eslint/no-explicit-any
        port = await (navigator as any).serial.requestPort({
          filters: [{ usbVendorId: 0x303A }],
        })
      }

      // eslint-disable-next-line @typescript-eslint/no-unsafe-argument
      const transport = new Transport(port)
      const esploader = new ESPLoader({ transport, baudrate: 921600 })
      await esploader.main()
      await esploader.flashId()
      espLoaderRef.current = esploader

      setState('bootloader_ready')
    } catch (error) {
      console.error('Bootloader entry failed:', error)
      toast({
        title:
          'Could not enter update mode. Try the manual method: hold BOOT while plugging in.',
        variant: 'error',
      })
      setState('error')
    }
  }, [output, isConnected, toast])

  const uploadFirmware = useCallback(
    async (firmwareUrl: string) => {
      if (!espLoaderRef.current) return

      setState('uploading')
      setProgress(0)

      try {
        const response = await fetch(firmwareUrl)
        if (!response.ok) throw new Error(`HTTP ${response.status}`)
        const blob = await response.blob()
        const firmwareData = new Uint8Array(await blob.arrayBuffer())

        await espLoaderRef.current.writeFlash({
          fileArray: [{ data: firmwareData, address: 0x10000 }],
          flashSize: 'keep',
          flashMode: 'keep',
          flashFreq: 'keep',
          eraseAll: false,
          compress: true,
          reportProgress: (
            _fileIndex: number,
            written: number,
            total: number,
          ) => {
            setProgress(Math.floor((written / total) * 100))
          },
        })

        toast({
          title:
            'Firmware updated successfully. Your T16 will restart.',
          variant: 'success',
        })
        setState('success')
      } catch (error) {
        console.error('Upload failed:', error)
        toast({
          title:
            'Upload failed. Unplug the device, plug it back in, and try again.',
          variant: 'error',
        })
        setState('error')
      } finally {
        setProgress(0)
      }
    },
    [toast],
  )

  const reset = useCallback(() => {
    setState('idle')
    setProgress(0)
    espLoaderRef.current = null
  }, [])

  return { state, progress, enterBootloader, uploadFirmware, reset, isConnected }
}
