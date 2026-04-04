// SysEx protocol constants — MUST match src/SysExProtocol.hpp exactly
export const MANUFACTURER_ID = 0x7d

export const CMD = {
  VERSION: 0x01,
  CONFIG: 0x02,
  PARAM: 0x03,
  CALIBRATION: 0x04,
  BOOTLOADER: 0x05,
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

export function requestVersion(output: Output): void {
  output.sendSysex(MANUFACTURER_ID, [CMD.VERSION, SUB.REQUEST])
}

export function requestConfigDump(output: Output): void {
  output.sendSysex(MANUFACTURER_ID, [CMD.CONFIG, SUB.REQUEST])
}

export function sendParamUpdate(
  output: Output,
  domain: number,
  bankIndex: number,
  fieldId: number,
  value: number,
): void {
  output.sendSysex(MANUFACTURER_ID, [CMD.PARAM, SUB.REQUEST, domain, bankIndex, fieldId, value])
}

export function requestBootloader(output: Output): void {
  output.sendSysex(MANUFACTURER_ID, [CMD.BOOTLOADER, SUB.REQUEST])
}

export function sendFullConfig(output: Output, config: object): void {
  const json = JSON.stringify(config)
  const data = Array.from(json).map((c) => c.charCodeAt(0))
  output.sendSysex(MANUFACTURER_ID, [CMD.CONFIG, SUB.LOAD, ...data])
}
