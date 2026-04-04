import { describe, it, expect } from 'vitest'
import { parseBLEMidiPacket, BLE_MIDI_SERVICE, BLE_MIDI_CHAR } from './ble'

function makeDataView(bytes: number[]): DataView {
  return new DataView(new Uint8Array(bytes).buffer)
}

describe('BLE MIDI constants', () => {
  it('exports correct GATT service UUID', () => {
    expect(BLE_MIDI_SERVICE).toBe('03b80e5a-ede8-4b33-a751-6ce34ec4c700')
  })

  it('exports correct GATT characteristic UUID', () => {
    expect(BLE_MIDI_CHAR).toBe('7772e5db-3868-4112-a1a9-f2669d106bf3')
  })
})

describe('parseBLEMidiPacket', () => {
  it('extracts a single Note On message', () => {
    // BLE packet: [header=0x80] [timestamp_low=0x80] [status=0x90] [note=0x3C] [vel=0x7F]
    const data = makeDataView([0x80, 0x80, 0x90, 0x3c, 0x7f])
    const messages = parseBLEMidiPacket(data)
    expect(messages).toHaveLength(1)
    expect(Array.from(messages[0])).toEqual([0x90, 0x3c, 0x7f])
  })

  it('extracts a single CC message', () => {
    // BLE packet: [header=0x80] [timestamp_low=0x80] [status=0xB0] [cc=0x4A] [val=0x40]
    const data = makeDataView([0x80, 0x80, 0xb0, 0x4a, 0x40])
    const messages = parseBLEMidiPacket(data)
    expect(messages).toHaveLength(1)
    expect(Array.from(messages[0])).toEqual([0xb0, 0x4a, 0x40])
  })

  it('returns empty array for header-only packet', () => {
    const data = makeDataView([0x80])
    const messages = parseBLEMidiPacket(data)
    expect(messages).toHaveLength(0)
  })

  it('extracts two messages from one packet', () => {
    // Two Note On messages:
    // [header=0x80] [ts=0x80] [0x90 0x3C 0x7F] [ts=0x81] [0x90 0x40 0x60]
    const data = makeDataView([0x80, 0x80, 0x90, 0x3c, 0x7f, 0x81, 0x90, 0x40, 0x60])
    const messages = parseBLEMidiPacket(data)
    expect(messages).toHaveLength(2)
    expect(Array.from(messages[0])).toEqual([0x90, 0x3c, 0x7f])
    expect(Array.from(messages[1])).toEqual([0x90, 0x40, 0x60])
  })

  it('handles Note Off message', () => {
    const data = makeDataView([0x80, 0x80, 0x80, 0x3c, 0x00])
    const messages = parseBLEMidiPacket(data)
    expect(messages).toHaveLength(1)
    expect(Array.from(messages[0])).toEqual([0x80, 0x3c, 0x00])
  })
})
