// MIDI service layer — pure functions, no React dependencies
// Handles all WebMIDI protocol logic: enable/disable, device discovery, SysEx encoding/decoding

import { WebMidi, type Input, type Output } from 'webmidi'
import {
  MANUFACTURER_ID,
  CMD,
  SUB,
  type SysExSender,
  requestVersion as sysexRequestVersion,
  requestConfigDump as sysexRequestConfigDump,
  requestCapabilities as sysexRequestCapabilities,
  sendParamUpdate as sysexSendParamUpdate,
  sendCCParamUpdate as sysexSendCCParamUpdate,
  sendFullConfig as sysexSendFullConfig,
  requestCalibration as sysexRequestCalibration,
  requestFactoryReset as sysexRequestFactoryReset,
} from '@/protocol/sysex'
import type { T16Configuration } from '@/types/config'
import type { Variant, Capabilities } from '@/types/variant'
import { isVariant } from '@/types/variant'

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
export function requestConfigDump(sender: SysExSender): void {
  sysexRequestConfigDump(sender)
}

/**
 * Request firmware version from the device.
 */
export function requestVersion(sender: SysExSender): void {
  sysexRequestVersion(sender)
}

/**
 * Request the device's variant + capabilities (Phase 11 firmware handshake).
 */
export function requestCapabilities(sender: SysExSender): void {
  sysexRequestCapabilities(sender)
}

/**
 * Send a single parameter update to the device.
 */
export function sendParamUpdate(
  sender: SysExSender,
  domain: number,
  bank: number,
  field: number,
  value: number,
): void {
  sysexSendParamUpdate(sender, domain, bank, field, value)
}

/**
 * Send a CC parameter update (channel + id) to the device.
 * CC domain requires both values in one message (firmware SetCCParam is atomic).
 */
export function sendCCParamUpdate(
  sender: SysExSender,
  bankIndex: number,
  ccIndex: number,
  channel: number,
  id: number,
): void {
  sysexSendCCParamUpdate(sender, bankIndex, ccIndex, channel, id)
}

/**
 * Send the full configuration to the device.
 */
export function sendFullConfig(sender: SysExSender, config: T16Configuration): void {
  sysexSendFullConfig(sender, config)
}

/**
 * Request calibration reset -- device will delete calibration data and restart.
 */
export function requestCalibration(sender: SysExSender): void {
  sysexRequestCalibration(sender)
}

/**
 * Request factory reset -- device will delete all data and restart.
 */
export function requestFactoryReset(sender: SysExSender): void {
  sysexRequestFactoryReset(sender)
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

/**
 * Check if a SysEx message is a CMD_CAPABILITIES response.
 * Mirrors the (cmd, sub) signature of isConfigResponse / isVersionResponse.
 */
export function isCapabilitiesResponse(cmd: number, sub: number): boolean {
  return cmd === CMD.CAPABILITIES && sub === SUB.RESPONSE
}

/**
 * Parse the JSON payload from a CMD_CAPABILITIES response.
 * Layout: bytes [0]=CMD, [1]=SUB, [2]=status, [3..]=ASCII JSON.
 * Returns null on any parse failure or unknown variant string —
 * caller treats null as "handshake unavailable, use fallback".
 */
export function parseCapabilitiesPayload(
  data: Uint8Array,
): { variant: Variant; capabilities: Capabilities } | null {
  if (data.length < 4) return null
  try {
    const json = String.fromCharCode(...Array.from(data.slice(3)))
    const parsed: unknown = JSON.parse(json)
    if (
      typeof parsed !== 'object' ||
      parsed === null ||
      !('variant' in parsed) ||
      !('capabilities' in parsed)
    )
      return null
    const v = (parsed as { variant: unknown }).variant
    const c = (parsed as { capabilities: unknown }).capabilities
    if (!isVariant(v)) return null
    if (
      typeof c !== 'object' ||
      c === null ||
      typeof (c as { touchSlider?: unknown }).touchSlider !== 'boolean' ||
      typeof (c as { koalaMode?: unknown }).koalaMode !== 'boolean'
    )
      return null
    return {
      variant: v,
      capabilities: {
        touchSlider: (c as { touchSlider: boolean }).touchSlider,
        koalaMode: (c as { koalaMode: boolean }).koalaMode,
      },
    }
  } catch {
    return null
  }
}
