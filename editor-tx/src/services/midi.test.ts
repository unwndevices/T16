import { describe, it, expect, vi } from 'vitest'
import { MANUFACTURER_ID, CMD, SUB, DOMAIN, FIELD_GLOBAL } from '@/protocol/sysex'

// Mock webmidi before importing midi service
vi.mock('webmidi', () => ({
  WebMidi: {
    enable: vi.fn().mockResolvedValue(undefined),
    disable: vi.fn().mockResolvedValue(undefined),
    getInputByName: vi.fn(),
    getOutputByName: vi.fn(),
    octaveOffset: 0,
  },
}))

import {
  parseSysExMessage,
  parseConfigDump,
  sendParamUpdate,
  sendCCParamUpdate,
  isConfigResponse,
  isVersionResponse,
  isParamAck,
} from '@/services/midi'

describe('parseSysExMessage', () => {
  it('extracts cmd, sub, and payload from valid SysEx data', () => {
    const data = new Uint8Array([
      0xf0, // status byte
      MANUFACTURER_ID, // manufacturer
      CMD.CONFIG, // command
      SUB.RESPONSE, // sub-command
      0x48,
      0x65,
      0x6c, // payload: "Hel"
    ])

    const result = parseSysExMessage(data)
    expect(result).not.toBeNull()
    expect(result!.cmd).toBe(CMD.CONFIG)
    expect(result!.sub).toBe(SUB.RESPONSE)
    expect(result!.payload).toEqual(new Uint8Array([0x48, 0x65, 0x6c]))
  })

  it('returns null for non-matching manufacturer ID', () => {
    const data = new Uint8Array([0xf0, 0x42, CMD.CONFIG, SUB.RESPONSE])
    const result = parseSysExMessage(data)
    expect(result).toBeNull()
  })

  it('handles empty payload', () => {
    const data = new Uint8Array([0xf0, MANUFACTURER_ID, CMD.VERSION, SUB.REQUEST])
    const result = parseSysExMessage(data)
    expect(result).not.toBeNull()
    expect(result!.cmd).toBe(CMD.VERSION)
    expect(result!.sub).toBe(SUB.REQUEST)
    expect(result!.payload.length).toBe(0)
  })
})

describe('parseConfigDump', () => {
  it('converts Uint8Array JSON to T16Configuration', () => {
    const config = {
      version: 200,
      global: {
        mode: 0,
        sensitivity: 1,
        brightness: 1,
        midi_trs: 0,
        trs_type: 0,
        passthrough: 0,
        midi_ble: 0,
        custom_scale1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
        custom_scale2: [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45],
      },
      banks: [],
    }
    const json = JSON.stringify(config)
    const payload = new Uint8Array(Array.from(json).map((c) => c.charCodeAt(0)))

    const result = parseConfigDump(payload)
    expect(result.version).toBe(200)
    expect(result.global.mode).toBe(0)
    expect(result.global.sensitivity).toBe(1)
  })
})

describe('sendParamUpdate', () => {
  it('sends correct SysEx byte sequence via output', () => {
    const mockOutput = {
      sendSysex: vi.fn(),
    }

    sendParamUpdate(
      mockOutput as unknown as Parameters<typeof sendParamUpdate>[0],
      DOMAIN.GLOBAL,
      0,
      FIELD_GLOBAL.BRIGHTNESS,
      3,
    )

    expect(mockOutput.sendSysex).toHaveBeenCalledWith(MANUFACTURER_ID, [
      CMD.PARAM,
      SUB.REQUEST,
      DOMAIN.GLOBAL,
      0,
      FIELD_GLOBAL.BRIGHTNESS,
      3,
    ])
  })
})

describe('sendCCParamUpdate', () => {
  it('sends 5-byte CC payload matching firmware expectation', () => {
    const sent: number[][] = []
    const mockOutput = {
      sendSysex: (_mfr: number, data: number[]) => {
        sent.push(data)
      },
    } as unknown as Parameters<typeof sendCCParamUpdate>[0]
    sendCCParamUpdate(mockOutput, 2, 3, 5, 64)
    expect(sent).toHaveLength(1)
    expect(sent[0]).toEqual([CMD.PARAM, SUB.REQUEST, DOMAIN.BANK_CC, 2, 3, 5, 64])
  })
})

describe('isConfigResponse', () => {
  it('returns true for config response', () => {
    expect(isConfigResponse(CMD.CONFIG, SUB.RESPONSE)).toBe(true)
  })

  it('returns false for other commands', () => {
    expect(isConfigResponse(CMD.VERSION, SUB.RESPONSE)).toBe(false)
    expect(isConfigResponse(CMD.CONFIG, SUB.REQUEST)).toBe(false)
  })
})

describe('isVersionResponse', () => {
  it('returns true for version response', () => {
    expect(isVersionResponse(CMD.VERSION, SUB.RESPONSE)).toBe(true)
  })
})

describe('isParamAck', () => {
  it('returns true for param ack', () => {
    expect(isParamAck(CMD.PARAM, SUB.ACK)).toBe(true)
  })
})
