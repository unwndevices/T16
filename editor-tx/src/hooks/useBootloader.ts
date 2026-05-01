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
        const timeout = setTimeout(() => reject(new Error('Bootloader entry timeout')), 5000)
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
      interface SerialPortInfo {
        usbVendorId?: number
        usbProductId?: number
      }
      interface WebSerialPort {
        getInfo(): SerialPortInfo
      }
      interface WebSerial {
        getPorts(): Promise<WebSerialPort[]>
        requestPort(opts?: { filters?: { usbVendorId: number }[] }): Promise<WebSerialPort>
      }
      const nav = navigator as Navigator & { serial: WebSerial }
      const ports = await nav.serial.getPorts()
      let port = ports.find((p) => {
        const info = p.getInfo()
        // ESP32-S3 USB Serial/JTAG: VID 0x303A, PID 0x1001
        return info.usbVendorId === 0x303a && info.usbProductId === 0x1001
      })

      if (!port) {
        // Fallback: prompt user to select the port
        port = await nav.serial.requestPort({
          filters: [{ usbVendorId: 0x303a }],
        })
      }

      const transport = new Transport(port as ConstructorParameters<typeof Transport>[0])
      const esploader = new ESPLoader({ transport, baudrate: 921600 })
      await esploader.main()
      await esploader.flashId()
      espLoaderRef.current = esploader

      setState('bootloader_ready')
    } catch (error) {
      console.error('Bootloader entry failed:', error)
      toast({
        title: 'Could not enter update mode. Try the manual method: hold BOOT while plugging in.',
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
          reportProgress: (_fileIndex: number, written: number, total: number) => {
            setProgress(Math.floor((written / total) * 100))
          },
        })

        toast({
          title: 'Firmware updated successfully. Your T16 will restart.',
          variant: 'success',
        })
        setState('success')
      } catch (error) {
        console.error('Upload failed:', error)
        toast({
          title: 'Upload failed. Unplug the device, plug it back in, and try again.',
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
