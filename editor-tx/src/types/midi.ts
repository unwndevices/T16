import type { Input, Output } from 'webmidi'
import type { T16Configuration } from '@/types/config'
import type { ImportResult } from '@/services/configValidator'

export interface ConnectionState {
  input: Input | null
  output: Output | null
  isConnected: boolean
  isDemo: boolean
}

export interface ConnectionContextValue extends ConnectionState {
  connect(): Promise<void>
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
  setConfig(config: T16Configuration): void
  importConfig(data: unknown): ImportResult
  exportConfig(): void
}

export type ConfigAction =
  | { type: 'SET_CONFIG'; payload: T16Configuration }
  | { type: 'SET_DEVICE_CONFIG'; payload: T16Configuration | null }
  | { type: 'UPDATE_PARAM'; domain: number; bank: number; field: number; value: number }
  | { type: 'SET_BANK'; payload: number }
  | { type: 'SYNC_CONFIRMED' }
