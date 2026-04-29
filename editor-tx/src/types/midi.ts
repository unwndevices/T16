import type { Input, Output } from 'webmidi'
import type { T16Configuration } from '@/types/config'
import type { ImportResult } from '@/services/configValidator'
import type { Variant, Capabilities } from '@/types/variant'

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
  variant: Variant
  capabilities: Capabilities
  isHandshakeConfirmed: boolean
  setVariant: (v: Variant) => void
  setCapabilities: (caps: Capabilities, fromHandshake: boolean) => void
  pendingAdaptation: PendingAdaptation | null
  confirmAdaptation: () => void
  cancelAdaptation: () => void
}

export interface PendingAdaptation {
  fileConfig: T16Configuration
  fileVariant: Variant
  deviceVariant: Variant
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
  | { type: 'SET_VARIANT'; payload: Variant }
  | { type: 'SET_CAPABILITIES'; payload: { capabilities: Capabilities; fromHandshake: boolean } }
  | { type: 'SET_PENDING_ADAPTATION'; payload: PendingAdaptation }
  | { type: 'CLEAR_PENDING_ADAPTATION' }
