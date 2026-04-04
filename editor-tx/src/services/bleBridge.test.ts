import { describe, it, expect, vi, beforeEach } from 'vitest'
import {
  frameSysExForBLE,
  BLESysExReassembler,
  BLEMidiTransport,
} from './bleBridge'

describe('frameSysExForBLE', () => {
  it('produces a single BLE MIDI packet for short SysEx', () => {
    const packets = frameSysExForBLE(0x7d, [0x01, 0x01])
    expect(packets).toHaveLength(1)
    const pkt = packets[0]
    // Must start with header (bit 7 set) + timestamp (bit 7 set)
    expect(pkt[0] & 0x80).toBe(0x80)
    expect(pkt[1] & 0x80).toBe(0x80)
    // Must contain F0, manufacturer, data, timestamp, F7
    expect(pkt[2]).toBe(0xf0)
    expect(pkt[3]).toBe(0x7d)
    expect(pkt[4]).toBe(0x01)
    expect(pkt[5]).toBe(0x01)
    // Second-to-last byte is timestamp (bit 7 set), last is F7
    expect(pkt[pkt.length - 1]).toBe(0xf7)
    expect(pkt[pkt.length - 2] & 0x80).toBe(0x80)
  })

  it('produces multiple packets for large SysEx', () => {
    // 18 data bytes + manufacturer + F0 + F7 = 21 bytes message, won't fit in single 20-byte packet
    const data = Array.from({ length: 18 }, (_, i) => i & 0x7f)
    const packets = frameSysExForBLE(0x7d, data)
    expect(packets.length).toBeGreaterThan(1)
  })

  it('ensures all packets are <= 20 bytes', () => {
    const data = Array.from({ length: 50 }, (_, i) => i & 0x7f)
    const packets = frameSysExForBLE(0x7d, data)
    for (const pkt of packets) {
      expect(pkt.length).toBeLessThanOrEqual(20)
    }
  })

  it('first packet starts with header+timestamp+F0', () => {
    const data = Array.from({ length: 50 }, (_, i) => i & 0x7f)
    const packets = frameSysExForBLE(0x7d, data)
    expect(packets[0][0] & 0x80).toBe(0x80) // header
    expect(packets[0][1] & 0x80).toBe(0x80) // timestamp
    expect(packets[0][2]).toBe(0xf0)         // SysEx start
  })

  it('continuation packets have header then data (byte 1 bit 7 clear)', () => {
    const data = Array.from({ length: 50 }, (_, i) => i & 0x7f)
    const packets = frameSysExForBLE(0x7d, data)
    // Middle packets (not first, not last)
    for (let p = 1; p < packets.length - 1; p++) {
      expect(packets[p][0] & 0x80).toBe(0x80) // header
      expect(packets[p][1] & 0x80).toBe(0)    // data byte, bit 7 clear
    }
  })

  it('last packet ends with timestamp+F7', () => {
    const data = Array.from({ length: 50 }, (_, i) => i & 0x7f)
    const packets = frameSysExForBLE(0x7d, data)
    const last = packets[packets.length - 1]
    expect(last[last.length - 1]).toBe(0xf7)
    expect(last[last.length - 2] & 0x80).toBe(0x80) // timestamp before F7
  })
})

describe('BLESysExReassembler', () => {
  let reassembler: BLESysExReassembler

  beforeEach(() => {
    reassembler = new BLESysExReassembler()
  })

  it('emits complete SysEx from single notification', () => {
    const msg = new Uint8Array([0xf0, 0x7d, 0x01, 0x02, 0xf7])
    const results = reassembler.feed([msg])
    expect(results).toHaveLength(1)
    expect(Array.from(results[0])).toEqual([0xf0, 0x7d, 0x01, 0x02, 0xf7])
  })

  it('accumulates SysEx across 3 notifications', () => {
    const part1 = new Uint8Array([0xf0, 0x7d, 0x01])
    const part2 = new Uint8Array([0x02, 0x03])
    const part3 = new Uint8Array([0x04, 0xf7])

    expect(reassembler.feed([part1])).toHaveLength(0)
    expect(reassembler.feed([part2])).toHaveLength(0)
    const results = reassembler.feed([part3])
    expect(results).toHaveLength(1)
    expect(Array.from(results[0])).toEqual([0xf0, 0x7d, 0x01, 0x02, 0x03, 0x04, 0xf7])
  })

  it('reset() clears partial buffer', () => {
    const part1 = new Uint8Array([0xf0, 0x7d, 0x01])
    reassembler.feed([part1])
    reassembler.reset()

    // After reset, a new SysEx should work independently
    const complete = new Uint8Array([0xf0, 0x7d, 0x05, 0xf7])
    const results = reassembler.feed([complete])
    expect(results).toHaveLength(1)
    expect(Array.from(results[0])).toEqual([0xf0, 0x7d, 0x05, 0xf7])
  })
})

describe('BLEMidiTransport', () => {
  let mockCharacteristic: {
    writeValueWithoutResponse: ReturnType<typeof vi.fn>
    addEventListener: ReturnType<typeof vi.fn>
    removeEventListener: ReturnType<typeof vi.fn>
  }
  let transport: BLEMidiTransport

  beforeEach(() => {
    mockCharacteristic = {
      writeValueWithoutResponse: vi.fn().mockResolvedValue(undefined),
      addEventListener: vi.fn(),
      removeEventListener: vi.fn(),
    }
    transport = new BLEMidiTransport(
      mockCharacteristic as unknown as BluetoothRemoteGATTCharacteristic,
    )
  })

  it('sendSysex calls writeValueWithoutResponse for each framed packet', () => {
    transport.sendSysex(0x7d, [0x01, 0x01])
    // At least one write call
    expect(mockCharacteristic.writeValueWithoutResponse).toHaveBeenCalled()
  })

  it('incoming notification dispatches to listener via reassembler', () => {
    const handler = vi.fn()
    transport.addSysexListener(handler)

    // Get the notification handler that was registered
    const [eventName, notifHandler] = mockCharacteristic.addEventListener.mock.calls[0]
    expect(eventName).toBe('characteristicvaluechanged')

    // Simulate a BLE MIDI notification with a complete SysEx
    // BLE packet: [header=0x80] [ts=0x80] [F0] [0x7D] [0x01] [ts=0x80] [F7]
    const blePacket = new DataView(
      new Uint8Array([0x80, 0x80, 0xf0, 0x7d, 0x01, 0x80, 0xf7]).buffer,
    )
    notifHandler({ target: { value: blePacket } })

    expect(handler).toHaveBeenCalledTimes(1)
    const received = handler.mock.calls[0][0] as Uint8Array
    expect(received[0]).toBe(0xf0)
    expect(received[received.length - 1]).toBe(0xf7)
  })

  it('removeSysexListener prevents further callbacks', () => {
    const handler = vi.fn()
    transport.addSysexListener(handler)
    transport.removeSysexListener(handler)

    // Simulate notification
    const [, notifHandler] = mockCharacteristic.addEventListener.mock.calls[0]
    const blePacket = new DataView(
      new Uint8Array([0x80, 0x80, 0xf0, 0x7d, 0x01, 0x80, 0xf7]).buffer,
    )
    notifHandler({ target: { value: blePacket } })

    expect(handler).not.toHaveBeenCalled()
  })

  it('dispose removes characteristic event listener', () => {
    transport.dispose()
    expect(mockCharacteristic.removeEventListener).toHaveBeenCalledWith(
      'characteristicvaluechanged',
      expect.any(Function),
    )
  })
})
