// SysEx protocol constants — MUST match src/SysExProtocol.hpp exactly
export const MANUFACTURER_ID = 0x7d

export const CMD = {
  VERSION: 0x01,
  CONFIG: 0x02,
  PARAM: 0x03,
  CALIBRATION: 0x04,
  BOOTLOADER: 0x05,
  FACTORY_RESET: 0x06,
  CAPABILITIES: 0x07,
} as const

export const SUB = {
  REQUEST: 0x01,
  RESPONSE: 0x02,
  LOAD: 0x03,
  ACK: 0x04,
} as const

export const DOMAIN = {
  GLOBAL: 0x00,
  BANK_KB: 0x01,
  BANK_CC: 0x02,
} as const

export const STATUS = {
  OK: 0x00,
  ERROR: 0x01,
} as const

// Global field IDs — match SysExProtocol.hpp FIELD_GLOBAL_* constants
export const FIELD_GLOBAL = {
  MODE: 0,
  SENSITIVITY: 1,
  BRIGHTNESS: 2,
  MIDI_TRS: 3,
  TRS_TYPE: 4,
  PASSTHROUGH: 5,
  MIDI_BLE: 6,
} as const

// Bank keyboard field IDs — match SysExProtocol.hpp FIELD_BANK_* constants
export const FIELD_BANK = {
  CHANNEL: 0,
  SCALE: 1,
  OCTAVE: 2,
  NOTE: 3,
  VELOCITY_CURVE: 4,
  AFTERTOUCH_CURVE: 5,
  FLIP_X: 6,
  FLIP_Y: 7,
  KOALA_MODE: 8,
} as const

// Helper functions
// Note: webmidi.js output.sendSysex(manufacturerId, data) automatically
// wraps with F0 <manufacturerId> ... F7. Do NOT include manufacturer ID in data array.

import type { Output } from 'webmidi'
import type { MidiTransport } from '@/types/midi'

/** A sender that supports sendSysex — either a WebMidi Output or a MidiTransport. */
export type SysExSender = Output | MidiTransport

export function requestVersion(sender: SysExSender): void {
  sender.sendSysex(MANUFACTURER_ID, [CMD.VERSION, SUB.REQUEST])
}

export function requestCapabilities(sender: SysExSender): void {
  sender.sendSysex(MANUFACTURER_ID, [CMD.CAPABILITIES, SUB.REQUEST])
}

export function requestConfigDump(sender: SysExSender): void {
  sender.sendSysex(MANUFACTURER_ID, [CMD.CONFIG, SUB.REQUEST])
}

export function sendParamUpdate(
  sender: SysExSender,
  domain: number,
  bankIndex: number,
  fieldId: number,
  value: number,
): void {
  sender.sendSysex(MANUFACTURER_ID, [CMD.PARAM, SUB.REQUEST, domain, bankIndex, fieldId, value])
}

export function sendCCParamUpdate(
  sender: SysExSender,
  bankIndex: number,
  ccIndex: number,
  channel: number,
  id: number,
): void {
  sender.sendSysex(MANUFACTURER_ID, [
    CMD.PARAM, SUB.REQUEST,
    DOMAIN.BANK_CC, bankIndex, ccIndex, channel, id
  ])
}

export function requestBootloader(sender: SysExSender): void {
  sender.sendSysex(MANUFACTURER_ID, [CMD.BOOTLOADER, SUB.REQUEST])
}

export function requestCalibration(sender: SysExSender): void {
  sender.sendSysex(MANUFACTURER_ID, [CMD.CALIBRATION, SUB.REQUEST])
}

export function requestFactoryReset(sender: SysExSender): void {
  sender.sendSysex(MANUFACTURER_ID, [CMD.FACTORY_RESET, SUB.REQUEST])
}

export function sendFullConfig(sender: SysExSender, config: object): void {
  const json = JSON.stringify(config)
  const data = Array.from(json).map((c) => c.charCodeAt(0))
  sender.sendSysex(MANUFACTURER_ID, [CMD.CONFIG, SUB.LOAD, ...data])
}
