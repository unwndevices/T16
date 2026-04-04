// MIDI service layer — pure functions, no React dependencies
// Handles all WebMIDI protocol logic: enable/disable, device discovery, SysEx encoding/decoding

import { WebMidi, type Input, type Output } from 'webmidi'
import {
  MANUFACTURER_ID,
  CMD,
  SUB,
  requestVersion as sysexRequestVersion,
  requestConfigDump as sysexRequestConfigDump,
  sendParamUpdate as sysexSendParamUpdate,
  sendCCParamUpdate as sysexSendCCParamUpdate,
  sendFullConfig as sysexSendFullConfig,
} from '@/protocol/sysex'
import type { T16Configuration } from '@/types/config'

const DEVICE_NAME = 'Topo T16'

/**
 * Enable WebMIDI with SysEx support.
 * Must be called from a user gesture (button click) in browsers.
 */
export async function enableMidi(): Promise<void> {
  await WebMidi.enable({ sysex: true })
  WebMidi.octaveOffset = -1
}

/**
 * Disable WebMIDI and release all resources.
 */
export async function disableMidi(): Promise<void> {
  await WebMidi.disable()
}

/**
 * Search for connected T16 device by name.
 * Returns input/output pair (either may be null if not found).
 */
export function findDevice(): { input: Input | null; output: Output | null } {
  const input = WebMidi.getInputByName(DEVICE_NAME) ?? null
  const output = WebMidi.getOutputByName(DEVICE_NAME) ?? null
  return { input, output }
}

/**
 * Request a full configuration dump from the device.
 */
export function requestConfigDump(output: Output): void {
  sysexRequestConfigDump(output)
}

/**
 * Request firmware version from the device.
 */
export function requestVersion(output: Output): void {
  sysexRequestVersion(output)
}

/**
 * Send a single parameter update to the device.
 */
export function sendParamUpdate(
  output: Output,
  domain: number,
  bank: number,
  field: number,
  value: number,
): void {
  sysexSendParamUpdate(output, domain, bank, field, value)
}

/**
 * Send a CC parameter update (channel + id) to the device.
 * CC domain requires both values in one message (firmware SetCCParam is atomic).
 */
export function sendCCParamUpdate(
  output: Output,
  bankIndex: number,
  ccIndex: number,
  channel: number,
  id: number,
): void {
  sysexSendCCParamUpdate(output, bankIndex, ccIndex, channel, id)
}

/**
 * Send the full configuration to the device.
 */
export function sendFullConfig(output: Output, config: T16Configuration): void {
  sysexSendFullConfig(output, config)
}

/**
 * Parse a raw SysEx message into its structured parts.
 * The data parameter is the raw Uint8Array from the sysex event,
 * which includes the status byte (0xF0) at index 0 and manufacturer ID at index 1.
 *
 * Returns null if the message is not from our manufacturer.
 */
export function parseSysExMessage(data: Uint8Array): {
  cmd: number
  sub: number
  payload: Uint8Array
} | null {
  // data[0] = 0xF0 (status byte, added by webmidi)
  // data[1] = manufacturer ID
  // data[2] = command
  // data[3] = sub-command
  // data[4..n-1] = payload
  // data[n] = 0xF7 (end, stripped by webmidi in some cases)

  if (data[1] !== MANUFACTURER_ID) {
    return null
  }

  const cmd = data[2]
  const sub = data[3]
  const payload = data.slice(4)

  return { cmd, sub, payload }
}

/**
 * Parse a config dump payload (SysEx data bytes) into a T16Configuration.
 * The payload is the raw bytes after CMD and SUB, representing JSON-encoded config.
 */
export function parseConfigDump(payload: Uint8Array): T16Configuration {
  const jsonString = Array.from(payload)
    .map((byte) => String.fromCharCode(byte))
    .join('')
  return JSON.parse(jsonString) as T16Configuration
}

/**
 * Check if a SysEx message is a config dump response.
 */
export function isConfigResponse(cmd: number, sub: number): boolean {
  return cmd === CMD.CONFIG && sub === SUB.RESPONSE
}

/**
 * Check if a SysEx message is a version response.
 */
export function isVersionResponse(cmd: number, sub: number): boolean {
  return cmd === CMD.VERSION && sub === SUB.RESPONSE
}

/**
 * Check if a SysEx message is a param acknowledgement.
 */
export function isParamAck(cmd: number, sub: number): boolean {
  return cmd === CMD.PARAM && sub === SUB.ACK
}
