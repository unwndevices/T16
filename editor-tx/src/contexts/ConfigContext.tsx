import { createContext, useReducer, useCallback, useEffect } from 'react'
import type { ReactNode } from 'react'
import {
  parseSysExMessage,
  parseConfigDump,
  isConfigResponse,
  sendParamUpdate as midiSendParamUpdate,
  requestConfigDump,
  requestVersion,
} from '@/services/midi'
import { DOMAIN, FIELD_GLOBAL, FIELD_BANK } from '@/protocol/sysex'
import type { T16Configuration } from '@/types/config'
import type { ConfigContextValue, ConfigAction } from '@/types/midi'
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
  chs: [1, 1, 1, 1, 1, 1, 1, 1] as [number, number, number, number, number, number, number, number],
  ids: [13, 14, 15, 16, 17, 18, 19, 20] as [number, number, number, number, number, number, number, number],
}

const DEFAULT_SCALE: [number, number, number, number, number, number, number, number, number, number, number, number, number, number, number, number] =
  [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]

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
    { ...DEFAULT_BANK, ids: [13, 14, 15, 16, 17, 18, 19, 20] },
    { ...DEFAULT_BANK, ids: [21, 22, 23, 24, 25, 26, 27, 28] },
    { ...DEFAULT_BANK, ids: [21, 22, 23, 24, 25, 26, 27, 28] },
    { ...DEFAULT_BANK, ids: [31, 32, 33, 34, 35, 36, 37, 38] },
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
      } else if (domain === DOMAIN.BANK_KB || domain === DOMAIN.BANK_CC) {
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

    case 'SET_BANK':
      return { ...state, selectedBank: action.payload }

    case 'SYNC_CONFIRMED':
      return { ...state, deviceConfig: structuredClone(state.config) }

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
  const { input, output, isConnected } = useConnection()

  const [state, dispatch] = useReducer(configReducer, {
    config: DEFAULT_CONFIG,
    deviceConfig: null,
    selectedBank: 0,
  })

  const isSynced = computeIsSynced(state.config, state.deviceConfig)

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
      if (output) {
        midiSendParamUpdate(output, domain, bank, field, value)
      }
    },
    [output],
  )

  // Request config dump and version when output becomes available
  useEffect(() => {
    if (!output || !isConnected) return
    requestConfigDump(output)
    requestVersion(output)
  }, [output, isConnected])

  // Listen for SysEx messages on input
  useEffect(() => {
    if (!input || !isConnected) return

    const handleSysex = (e: { data: Uint8Array }) => {
      const msg = parseSysExMessage(e.data)
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
    }

    input.addListener('sysex', handleSysex)
    return () => {
      input.removeListener('sysex', handleSysex)
    }
  }, [input, isConnected])

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
    setConfig,
  }

  return (
    <ConfigContext.Provider value={value}>
      {children}
    </ConfigContext.Provider>
  )
}

export default ConfigContext
