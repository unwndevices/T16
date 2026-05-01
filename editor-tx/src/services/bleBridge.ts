// BLE MIDI bridge — SysEx framing, reassembly, and transport abstraction
// BLE MIDI spec: packets are max 20 bytes (default ATT MTU 23 - 3 overhead)

import { parseBLEMidiPacket } from './ble'
import type { MidiTransport } from '@/types/midi'

/** Conservative max BLE MIDI packet size (default ATT MTU 23 - 3 ATT overhead) */
const BLE_PACKET_MAX = 20

/**
 * Generate BLE MIDI timestamp bytes from current time.
 * Header: bit 7 set, bits 5-0 = timestamp high 6 bits
 * TsLow: bit 7 set, bits 6-0 = timestamp low 7 bits
 */
function getMidiTimestamp(): [number, number] {
  const ts = Date.now() & 0x1fff
  const header = ((ts >> 7) & 0x3f) | 0x80
  const low = (ts & 0x7f) | 0x80
  return [header, low]
}

/**
 * Frame a SysEx message into BLE MIDI packets.
 *
 * @param manufacturerId - MIDI manufacturer ID (e.g. 0x7D)
 * @param data - SysEx data bytes (without F0/F7 wrapping)
 * @returns Array of Uint8Array packets ready for characteristic.writeValueWithoutResponse()
 *
 * BLE MIDI packet format:
 * - Single packet: [header] [ts] [F0 mfr data...] [ts] [F7]
 * - First of multi: [header] [ts] [F0 mfr data...]
 * - Continuation:   [header] [data...] (byte 1 bit 7 clear = continuation)
 * - Last of multi:  [header] [data...] [ts] [F7]
 */
export function frameSysExForBLE(manufacturerId: number, data: number[]): Uint8Array[] {
  const fullMessage = [0xf0, manufacturerId, ...data, 0xf7]
  const packets: Uint8Array[] = []

  // Check if entire message fits in a single packet
  // Single packet format: header(1) + ts(1) + message_without_F7 + ts(1) + F7(1)
  // = 2 + (fullMessage.length - 1) + 1 + 1 = fullMessage.length + 3
  // Wait - the message includes F7 already. Single packet:
  // header(1) + ts(1) + F0...data(N-1) + ts(1) + F7(1) = N + 3
  // where N = fullMessage.length (includes both F0 and F7)
  // So total = fullMessage.length - 1 + 4 = fullMessage.length + 3
  // No: header + ts + [F0, mfr, data...] + ts + F7
  // The [F0, mfr, data...] is fullMessage without the trailing F7: length - 1
  // Total: 2 + (fullMessage.length - 1) + 1 + 1 = fullMessage.length + 3
  // Hmm, that's: 2 + len-1 + 2 = len + 3. No: 2 + (len-1) + 1 + 1 = len + 3. Yes.

  if (fullMessage.length + 3 <= BLE_PACKET_MAX) {
    // Single packet: [header] [ts] [F0 mfr data...] [ts] [F7]
    const [header, tsLow] = getMidiTimestamp()
    const bodyWithoutF7 = fullMessage.slice(0, -1) // everything except F7
    const pkt = new Uint8Array(2 + bodyWithoutF7.length + 2) // header+ts + body + ts+F7
    pkt[0] = header
    pkt[1] = tsLow
    for (let i = 0; i < bodyWithoutF7.length; i++) {
      pkt[2 + i] = bodyWithoutF7[i]
    }
    pkt[pkt.length - 2] = tsLow
    pkt[pkt.length - 1] = 0xf7
    packets.push(pkt)
    return packets
  }

  // Multi-packet framing
  let offset = 0

  // First packet: [header] [ts] [F0 data...]
  {
    const [header, tsLow] = getMidiTimestamp()
    const capacity = BLE_PACKET_MAX - 2 // reserve header + timestamp
    // Grab as much of the full message as fits
    const endOffset = Math.min(offset + capacity, fullMessage.length)
    const chunk = fullMessage.slice(offset, endOffset)
    const pkt = new Uint8Array(2 + chunk.length)
    pkt[0] = header
    pkt[1] = tsLow
    for (let i = 0; i < chunk.length; i++) {
      pkt[2 + i] = chunk[i]
    }
    packets.push(pkt)
    offset = endOffset
  }

  // Continuation and last packets
  while (offset < fullMessage.length) {
    const [header] = getMidiTimestamp()
    // Check if this is the last packet (will contain F7)
    // Last packet needs: header(1) + data... + ts(1) + F7(1)
    // If remaining data includes F7 at end, we need to insert ts before it
    const isLast = offset + (BLE_PACKET_MAX - 1) >= fullMessage.length

    if (isLast) {
      // Last packet: [header] [data...] [ts] [F7]
      const [, tsLow] = getMidiTimestamp()
      // Data is everything except the F7 at the end
      const dataBytes = fullMessage.slice(offset, fullMessage.length - 1) // exclude F7
      const pkt = new Uint8Array(1 + dataBytes.length + 2) // header + data + ts + F7
      pkt[0] = header
      for (let i = 0; i < dataBytes.length; i++) {
        pkt[1 + i] = dataBytes[i]
      }
      pkt[pkt.length - 2] = tsLow
      pkt[pkt.length - 1] = 0xf7
      packets.push(pkt)
      offset = fullMessage.length
    } else {
      // Continuation packet: [header] [data...] (no timestamp before data)
      const capacity = BLE_PACKET_MAX - 1 // header only
      const endOffset = Math.min(offset + capacity, fullMessage.length - 1) // don't consume F7 yet
      const chunk = fullMessage.slice(offset, endOffset)
      const pkt = new Uint8Array(1 + chunk.length)
      pkt[0] = header
      for (let i = 0; i < chunk.length; i++) {
        pkt[1 + i] = chunk[i]
      }
      packets.push(pkt)
      offset = endOffset
    }
  }

  return packets
}

/**
 * Reassembles SysEx messages from BLE MIDI notification streams.
 * BLE notifications may split a SysEx across multiple packets.
 * This class accumulates data until F7 is received.
 */
export class BLESysExReassembler {
  private buffer: number[] = []
  private collecting = false

  /**
   * Feed parsed MIDI messages from a BLE notification.
   * @param messages - Individual MIDI messages extracted by parseBLEMidiPacket
   * @returns Array of complete SysEx messages (empty if none completed)
   */
  feed(messages: Uint8Array[]): Uint8Array[] {
    const complete: Uint8Array[] = []

    for (const msg of messages) {
      if (msg.length === 0) continue

      if (msg[0] === 0xf0) {
        // Start of new SysEx
        this.buffer = Array.from(msg)
        this.collecting = true

        if (msg[msg.length - 1] === 0xf7) {
          complete.push(new Uint8Array(this.buffer))
          this.buffer = []
          this.collecting = false
        }
      } else if (this.collecting) {
        // Continuation data
        this.buffer.push(...msg)

        if (msg[msg.length - 1] === 0xf7) {
          complete.push(new Uint8Array(this.buffer))
          this.buffer = []
          this.collecting = false
        }
      }
    }

    return complete
  }

  /** Clear partial buffer (e.g. on disconnect) */
  reset(): void {
    this.buffer = []
    this.collecting = false
  }
}

/**
 * BLE MIDI transport that wraps a GATT characteristic into the MidiTransport interface.
 * Handles SysEx framing for outgoing writes and reassembly for incoming notifications.
 */
export class BLEMidiTransport implements MidiTransport {
  private reassembler = new BLESysExReassembler()
  private listeners = new Set<(data: Uint8Array) => void>()
  private notificationHandler: (event: Event) => void

  constructor(private characteristic: BluetoothRemoteGATTCharacteristic) {
    this.notificationHandler = (event: Event) => {
      const target = (event as unknown as { target: { value: DataView } }).target
      const messages = parseBLEMidiPacket(target.value)
      const completeSysex = this.reassembler.feed(messages)
      for (const sysex of completeSysex) {
        for (const handler of this.listeners) {
          handler(sysex)
        }
      }
    }
    this.characteristic.addEventListener('characteristicvaluechanged', this.notificationHandler)
  }

  /**
   * Send a SysEx message over BLE.
   * Frames the message into BLE MIDI packets and writes each sequentially.
   */
  sendSysex(manufacturerId: number, data: number[]): void {
    const packets = frameSysExForBLE(manufacturerId, data)
    // Fire off sequential writes -- each must complete before the next
    void this.writePackets(packets)
  }

  private async writePackets(packets: Uint8Array[]): Promise<void> {
    for (const pkt of packets) {
      await this.characteristic.writeValueWithoutResponse(pkt as unknown as BufferSource)
    }
  }

  addSysexListener(handler: (data: Uint8Array) => void): void {
    this.listeners.add(handler)
  }

  removeSysexListener(handler: (data: Uint8Array) => void): void {
    this.listeners.delete(handler)
  }

  dispose(): void {
    this.characteristic.removeEventListener('characteristicvaluechanged', this.notificationHandler)
    this.listeners.clear()
    this.reassembler.reset()
  }
}
