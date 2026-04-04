import type { Input, Output } from 'webmidi'
import type { T16Configuration } from '@/types/config'
import type { ImportResult } from '@/services/configValidator'

export interface ConnectionState {
  input: Input | null
  output: Output | null
  transport: MidiTransport | null
  isConnected: boolean
  isDemo: boolean
}

export interface ConnectionContextValue extends ConnectionState {
  connect(): Promise<void>
  connectBLE(): Promise<void>
  disconnect(): void
  setDemo(demo: boolean): void
}

export interface ConfigState {
  config: T16Configuration
  deviceConfig: T16Configuration | null
  selectedBank: number
  isSynced: boolean
}

export interface ConfigContextValue extends ConfigState {
  setBank(bank: number): void
  updateParam(domain: number, bank: number, field: number, value: number): void
  updateCCParam(bank: number, ccIndex: number, channel: number, id: number): void
  setConfig(config: T16Configuration): void
  importConfig(data: unknown): ImportResult
  exportConfig(): void
}

export interface MidiTransport {
  sendSysex(manufacturerId: number, data: number[]): void
  addSysexListener(handler: (data: Uint8Array) => void): void
  removeSysexListener(handler: (data: Uint8Array) => void): void
  dispose(): void
}

export type ConfigAction =
  | { type: 'SET_CONFIG'; payload: T16Configuration }
  | { type: 'SET_DEVICE_CONFIG'; payload: T16Configuration | null }
  | { type: 'UPDATE_PARAM'; domain: number; bank: number; field: number; value: number }
  | { type: 'UPDATE_CC_PARAM'; bank: number; ccIndex: number; channel: number; id: number }
  | { type: 'SET_BANK'; payload: number }
