import { createContext, useState, useCallback, useEffect, useRef } from 'react'
import type { ReactNode } from 'react'
import type { Input, Output } from 'webmidi'
import { enableMidi, disableMidi, findDevice } from '@/services/midi'
import type { ConnectionContextValue } from '@/types/midi'

const ConnectionContext = createContext<ConnectionContextValue | null>(null)

interface ConnectionProviderProps {
  children: ReactNode
}

export function ConnectionProvider({ children }: ConnectionProviderProps) {
  const [input, setInput] = useState<Input | null>(null)
  const [output, setOutput] = useState<Output | null>(null)
  const [isConnected, setIsConnected] = useState(false)
  const [isDemo, setIsDemo] = useState(false)

  // Track MIDIAccess for statechange listener cleanup
  const midiAccessRef = useRef<MIDIAccess | null>(null)
  const stateChangeHandlerRef = useRef<((e: Event) => void) | null>(null)

  const setDemo = useCallback((demo: boolean) => {
    setIsDemo(demo)
    if (demo) {
      setIsConnected(true)
    }
  }, [])

  const disconnect = useCallback(() => {
    disableMidi()
    setInput(null)
    setOutput(null)
    setIsConnected(false)
  }, [])

  const connect = useCallback(async () => {
    if (isDemo) {
      setIsConnected(true)
      return
    }

    try {
      await enableMidi()

      // Listen for device connect/disconnect events via MIDIAccess
      const access = await navigator.requestMIDIAccess({ sysex: true })
      midiAccessRef.current = access

      const handleStateChange = (event: Event) => {
        const midiEvent = event as MIDIConnectionEvent
        const port = midiEvent.port
        if (!port || port.name !== 'Topo T16') return

        if (port.state === 'connected') {
          // Re-discover device when reconnected
          const device = findDevice()
          if (device.input) setInput(device.input)
          if (device.output) setOutput(device.output)
          if (device.input) setIsConnected(true)
        } else if (port.state === 'disconnected') {
          disconnect()
        }
      }

      stateChangeHandlerRef.current = handleStateChange
      access.addEventListener('statechange', handleStateChange)

      // Initial device discovery
      const device = findDevice()
      setInput(device.input)
      setOutput(device.output)

      const connected = device.input !== null && device.input.connection === 'open'
      setIsConnected(connected)
    } catch (err) {
      console.error('Error enabling WebMidi:', err)
      setIsConnected(false)
      throw err
    }
  }, [isDemo, disconnect])

  // Cleanup MIDIAccess listener on unmount
  useEffect(() => {
    return () => {
      if (midiAccessRef.current && stateChangeHandlerRef.current) {
        midiAccessRef.current.removeEventListener('statechange', stateChangeHandlerRef.current)
      }
    }
  }, [])

  // Listen for input state changes when input is available
  useEffect(() => {
    if (!input) return

    const handleStateChange = () => {
      setIsConnected(input.connection === 'open')
    }

    // webmidi.js uses 'disconnected' event on Input, not 'statechange'
    input.addListener('disconnected', handleStateChange)
    return () => {
      input.removeListener('disconnected', handleStateChange)
    }
  }, [input])

  const value: ConnectionContextValue = {
    input,
    output,
    isConnected,
    isDemo,
    connect,
    disconnect,
    setDemo,
  }

  return (
    <ConnectionContext.Provider value={value}>
      {children}
    </ConnectionContext.Provider>
  )
}

export default ConnectionContext
