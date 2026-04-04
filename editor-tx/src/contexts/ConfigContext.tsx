import { createContext, useReducer, useCallback, useEffect, useRef } from 'react'
import type { ReactNode } from 'react'
import {
  parseSysExMessage,
  parseConfigDump,
  isConfigResponse,
  sendParamUpdate as midiSendParamUpdate,
  sendCCParamUpdate as midiSendCCParamUpdate,
  sendFullConfig,
  requestConfigDump,
  requestVersion,
  isParamAck,
} from '@/services/midi'
import { DOMAIN, FIELD_GLOBAL, FIELD_BANK } from '@/protocol/sysex'
import type { T16Configuration } from '@/types/config'
import type { ConfigContextValue, ConfigAction } from '@/types/midi'
import { prepareImport, type ImportResult } from '@/services/configValidator'
import { useConnection } from '@/hooks/useConnection'

// Default configuration matching the device defaults
const DEFAULT_BANK = {
  ch: 1,
  scale: 0,
  oct: 0,
  note: 0,
  vel: 0,
  at: 0,
  flip_x: 0,
  flip_y: 0,
  koala_mode: 0,
  pal: 0,
  chs: [1, 1, 1, 1, 1, 1, 1, 1] as [number, number, number, number, number, number, number, number],
  ids: [13, 14, 15, 16, 17, 18, 19, 20] as [
    number,
    number,
    number,
    number,
    number,
    number,
    number,
    number,
  ],
}

const DEFAULT_SCALE: [
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]

export const DEFAULT_CONFIG: T16Configuration = {
  version: 200,
  global: {
    mode: 0,
    sensitivity: 1,
    brightness: 1,
    midi_trs: 0,
    trs_type: 0,
    passthrough: 0,
    midi_ble: 0,
    custom_scale1: DEFAULT_SCALE,
    custom_scale2: [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45],
  },
  banks: [
    { ...DEFAULT_BANK, pal: 0, ids: [13, 14, 15, 16, 17, 18, 19, 20] },
    { ...DEFAULT_BANK, pal: 1, ids: [21, 22, 23, 24, 25, 26, 27, 28] },
    { ...DEFAULT_BANK, pal: 2, ids: [21, 22, 23, 24, 25, 26, 27, 28] },
    { ...DEFAULT_BANK, pal: 3, ids: [31, 32, 33, 34, 35, 36, 37, 38] },
  ],
}

interface ConfigReducerState {
  config: T16Configuration
  deviceConfig: T16Configuration | null
  selectedBank: number
}

// Map global field IDs to config property names
const GLOBAL_FIELD_MAP: Record<number, keyof T16Configuration['global']> = {
  [FIELD_GLOBAL.MODE]: 'mode',
  [FIELD_GLOBAL.SENSITIVITY]: 'sensitivity',
  [FIELD_GLOBAL.BRIGHTNESS]: 'brightness',
  [FIELD_GLOBAL.MIDI_TRS]: 'midi_trs',
  [FIELD_GLOBAL.TRS_TYPE]: 'trs_type',
  [FIELD_GLOBAL.PASSTHROUGH]: 'passthrough',
  [FIELD_GLOBAL.MIDI_BLE]: 'midi_ble',
}

// Map bank field IDs to config property names
const BANK_FIELD_MAP: Record<number, string> = {
  [FIELD_BANK.CHANNEL]: 'ch',
  [FIELD_BANK.SCALE]: 'scale',
  [FIELD_BANK.OCTAVE]: 'oct',
  [FIELD_BANK.NOTE]: 'note',
  [FIELD_BANK.VELOCITY_CURVE]: 'vel',
  [FIELD_BANK.AFTERTOUCH_CURVE]: 'at',
  [FIELD_BANK.FLIP_X]: 'flip_x',
  [FIELD_BANK.FLIP_Y]: 'flip_y',
  [FIELD_BANK.KOALA_MODE]: 'koala_mode',
}

function configReducer(state: ConfigReducerState, action: ConfigAction): ConfigReducerState {
  switch (action.type) {
    case 'SET_CONFIG':
      return { ...state, config: action.payload }

    case 'SET_DEVICE_CONFIG':
      return { ...state, deviceConfig: action.payload }

    case 'UPDATE_PARAM': {
      const { domain, bank, field, value } = action
      const newConfig = structuredClone(state.config)

      if (domain === DOMAIN.GLOBAL) {
        const key = GLOBAL_FIELD_MAP[field]
        if (key && key !== 'custom_scale1' && key !== 'custom_scale2') {
          // Safe to cast: we've excluded tuple fields above
          ;(newConfig.global as unknown as Record<string, number>)[key] = value
        }
      } else if (domain === DOMAIN.BANK_KB) {
        const bankConfig = newConfig.banks[bank]
        if (bankConfig) {
          const key = BANK_FIELD_MAP[field]
          if (key) {
            ;(bankConfig as Record<string, unknown>)[key] = value
          }
        }
      }

      return { ...state, config: newConfig }
    }

    case 'UPDATE_CC_PARAM': {
      const { bank, ccIndex, channel, id } = action
      const newConfig = structuredClone(state.config)
      const bankConfig = newConfig.banks[bank]
      if (bankConfig && ccIndex >= 0 && ccIndex < 8) {
        bankConfig.chs[ccIndex] = channel
        bankConfig.ids[ccIndex] = id
      }
      return { ...state, config: newConfig }
    }

    case 'SET_BANK':
      return { ...state, selectedBank: action.payload }

    default:
      return state
  }
}

function computeIsSynced(config: T16Configuration, deviceConfig: T16Configuration | null): boolean {
  if (!deviceConfig) return false
  return JSON.stringify(config) === JSON.stringify(deviceConfig)
}

const ConfigContext = createContext<ConfigContextValue | null>(null)

interface ConfigProviderProps {
  children: ReactNode
}

export function ConfigProvider({ children }: ConfigProviderProps) {
  const { input, output, transport, isConnected } = useConnection()

  const [state, dispatch] = useReducer(configReducer, {
    config: DEFAULT_CONFIG,
    deviceConfig: null,
    selectedBank: 0,
  })

  const isSynced = computeIsSynced(state.config, state.deviceConfig)

  // Track pending param send timestamps for round-trip measurement
  const pendingParamTimestamps = useRef<Map<string, number>>(new Map())

  // Derive the active sender: BLE transport takes priority, fallback to USB output
  const sender = transport ?? output

  // Keep a ref for sender to avoid stale closures in timeouts
  const senderRef = useRef(sender)
  senderRef.current = sender

  // Shared SysEx message handler used by both USB and BLE receive paths
  const handleSysexData = useCallback((data: Uint8Array) => {
    const msg = parseSysExMessage(data)
    if (!msg) return

    if (isConfigResponse(msg.cmd, msg.sub)) {
      try {
        const config = parseConfigDump(msg.payload)
        dispatch({ type: 'SET_CONFIG', payload: config })
        dispatch({ type: 'SET_DEVICE_CONFIG', payload: config })
      } catch (err) {
        console.error('Failed to parse config dump:', err)
      }
    }

    // Measure per-parameter sync round-trip time
    if (isParamAck(msg.cmd, msg.sub)) {
      const timestamps = pendingParamTimestamps.current
      if (timestamps.size > 0) {
        const entry = timestamps.entries().next()
        if (!entry.done) {
          const [key, sendTime] = entry.value
          const roundTrip = performance.now() - sendTime
          console.debug(`[T16 Sync] Param ${key} round-trip: ${roundTrip.toFixed(1)}ms`)
          timestamps.delete(key)
        }
      }
    }
  }, [])

  const setBank = useCallback((bank: number) => {
    dispatch({ type: 'SET_BANK', payload: bank })
  }, [])

  const setConfig = useCallback((config: T16Configuration) => {
    dispatch({ type: 'SET_CONFIG', payload: config })
  }, [])

  const updateParam = useCallback(
    (domain: number, bank: number, field: number, value: number) => {
      dispatch({ type: 'UPDATE_PARAM', domain, bank, field, value })

      // Send to device if connected
      const s = senderRef.current
      if (s) {
        const key = `${domain}-${bank}-${field}`
        pendingParamTimestamps.current.set(key, performance.now())
        midiSendParamUpdate(s, domain, bank, field, value)

        // Retry after 500ms if no ACK received
        setTimeout(() => {
          if (pendingParamTimestamps.current.has(key)) {
            console.debug(`[T16 Sync] Param ${key} no ACK, retrying...`)
            const retryS = senderRef.current
            if (retryS) midiSendParamUpdate(retryS, domain, bank, field, value)
            // Final failure after another 500ms
            setTimeout(() => {
              if (pendingParamTimestamps.current.has(key)) {
                pendingParamTimestamps.current.delete(key)
                console.warn(`[T16 Sync] Param ${key} sync failed after retry`)
              }
            }, 500)
          }
        }, 500)
      }
    },
    [sender],
  )

  const updateCCParam = useCallback(
    (bank: number, ccIndex: number, channel: number, id: number) => {
      dispatch({ type: 'UPDATE_CC_PARAM', bank, ccIndex, channel, id })

      const s = senderRef.current
      if (s) {
        const key = `cc-${bank}-${ccIndex}`
        pendingParamTimestamps.current.set(key, performance.now())
        midiSendCCParamUpdate(s, bank, ccIndex, channel, id)

        setTimeout(() => {
          if (pendingParamTimestamps.current.has(key)) {
            console.debug(`[T16 Sync] CC param ${key} no ACK, retrying...`)
            const retryS = senderRef.current
            if (retryS) midiSendCCParamUpdate(retryS, bank, ccIndex, channel, id)
            setTimeout(() => {
              if (pendingParamTimestamps.current.has(key)) {
                pendingParamTimestamps.current.delete(key)
                console.warn(`[T16 Sync] CC param ${key} sync failed after retry`)
              }
            }, 500)
          }
        }, 500)
      }
    },
    [sender],
  )

  const importConfig = useCallback(
    (data: unknown): ImportResult => {
      const result = prepareImport(data)
      if (result.valid && result.config) {
        dispatch({ type: 'SET_CONFIG', payload: result.config })
        const s = senderRef.current
        if (s) {
          sendFullConfig(s, result.config)
        }
      }
      return result
    },
    [sender],
  )

  const exportConfig = useCallback(() => {
    const exportData = {
      ...state.config,
      _schema_version: 200,
      _exported_at: new Date().toISOString(),
    }
    const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `t16-config-${Date.now()}.topo`
    a.click()
    URL.revokeObjectURL(url)
  }, [state.config])

  // Request config dump and version when a sender becomes available
  useEffect(() => {
    const s = transport ?? output
    if (!s || !isConnected) return
    requestConfigDump(s)
    requestVersion(s)
  }, [output, transport, isConnected])

  // Listen for SysEx messages on USB input
  useEffect(() => {
    if (!input || !isConnected) return

    const handleSysex = (e: { data: Uint8Array }) => {
      handleSysexData(e.data)
    }

    input.addListener('sysex', handleSysex)
    return () => {
      input.removeListener('sysex', handleSysex)
    }
  }, [input, isConnected, handleSysexData])

  // Listen for SysEx messages on BLE transport (when no USB input available)
  useEffect(() => {
    if (!transport || !isConnected || input) return
    // If USB input exists, the USB path handles SysEx -- skip BLE listener to avoid duplicates

    transport.addSysexListener(handleSysexData)
    return () => {
      transport.removeSysexListener(handleSysexData)
    }
  }, [transport, isConnected, input, handleSysexData])

  // Clear device config on disconnect
  useEffect(() => {
    if (!isConnected) {
      dispatch({ type: 'SET_DEVICE_CONFIG', payload: null })
    }
  }, [isConnected])

  const value: ConfigContextValue = {
    config: state.config,
    deviceConfig: state.deviceConfig,
    selectedBank: state.selectedBank,
    isSynced,
    setBank,
    updateParam,
    updateCCParam,
    setConfig,
    importConfig,
    exportConfig,
  }

  return <ConfigContext.Provider value={value}>{children}</ConfigContext.Provider>
}

export default ConfigContext
